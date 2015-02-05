<?php

/**
 * Notify handler.
 *
 * Receives POST from PayWay Net with transaction details (encrypted).
 */
function commerce_payway_net_notify_handler() {
  //watchdog('commerce_payway_net', 'Notify: <pre>!dbg</pre>', array('!dbg' => print_r($dbg,1)), WATCHDOG_DEBUG);
  //error_log('Post: ' . print_r($_POST,1));

  // Get Order associated with this Transaction
  if (empty($_POST['payment_reference'])) {
    // no payment reference supplied,
    // return access denied & log error
    watchdog('commerce_payway_net', 'Payment notification lacks order reference.', array(), WATCHDOG_NOTICE);
    drupal_not_found();
    return;
  }

  if (!$order = commerce_order_load_by_number($_POST['payment_reference'])) {
    // no transaction found for supplied reference
    // return access denied & log error
    watchdog('commerce_payway_net', 'Unable to locate order @ref matching payment notification data.', array('@ref' => $_POST['payment_reference']), WATCHDOG_NOTICE);
    drupal_not_found();
    return;
  }

  // Get Payment Method associated with Order
  if (!isset($order->data['payment_method'])) {
    // no payment method on order
    // return access denied & log error
    watchdog('commerce_payway_net', 'Unable to locate payment method for order @ref.', array('@ref' => $_POST['payment_reference']), WATCHDOG_ERROR);
    drupal_not_found();
    return;
  }

  if (!$method = commerce_payment_method_instance_load($order->data['payment_method'])) {
    // no payment method on order
    watchdog('commerce_payway_net', 'Unable to load payment method for order @ref.', array('@ref' => $_POST['payment_reference']), WATCHDOG_ERROR);
    drupal_not_found();
    return;
  }

  // Authenticate notification using user/pass from payment method
  if (!isset($method['settings']['commerce_payway_net_username']) || !isset($method['settings']['commerce_payway_net_password'])) {
    // no auth settings to check
    watchdog('commerce_payway_net', 'Authentication not configured for payment method @id.', array('@id' => $method['instance_id']), WATCHDOG_ERROR);
    drupal_not_found();
    return;
  }

  if ($_POST['username'] != $method['settings']['commerce_payway_net_username'] || $_POST['password'] != $method['settings']['commerce_payway_net_password']) {
    // no auth settings to check
    watchdog('commerce_payway_net', 'Incorrect auth data submitted against order @ref.', array('@ref' => $_POST['payment_reference']), WATCHDOG_ERROR);
    drupal_not_found();
    return;
  }

  // Unsure why PayWay Net returns the amount with a thousands separator,
  // but there you have it.
  $_POST['am_payment'] = str_replace(',', '', $_POST['am_payment']);

  if ($_POST['fl_success'] == '1') {
    watchdog('commerce_payway_net', 'Payment success reported by PayWay Net for order @order_id', array('@order_id' => $order->order_id), WATCHDOG_NOTICE);
    $transaction = commerce_payment_transaction_new('commerce_payway_net', $order->order_id);
    $transaction->instance_id = $method['instance_id'];
    $transaction->remote_id = $_POST['id_batch_entry'];
    $transaction->amount = $_POST['am_payment'] * 100;
    $transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
    $transaction->message = 'Paid via PayWay Net.';
    commerce_payment_transaction_save($transaction);
    //error_log('Transaction: ' . print_r($transaction,1));
    print 'SUCCESSFUL NOTIFICATION ACCEPTED';
    // Switch user for correct perms when generating the order items view for
    // the message. if this isn't done the items will be missing because the
    // current user is probably anonymous as is callback from Payway.
    // See https://drupal.org/node/1895418#comment-7422568
    global $user;
    $original_user = $user;
    $old_state = drupal_save_session(FALSE);
    $user = user_load(1);
    // Progress the order status from Checkout:Payment to Completed.
    commerce_payment_redirect_pane_next_page($order);
    // Switch back to original user
    $user = $original_user;
    drupal_save_session($old_state);
    // Exit
    drupal_exit();
  }
  else {
    watchdog('commerce_payway_net', 'Payment failure reported by PayWay Net for order @order_id', array('@order_id' => $order->order_id), WATCHDOG_ERROR);
    $transaction = commerce_payment_transaction_new('commerce_payway_net', $order->order_id);
    $transaction->instance_id = $method['instance_id'];
    $transaction->remote_id = $_POST['id_batch_entry'];
    $transaction->amount = $_POST['am_payment'] * 100;
    $transaction->status = COMMERCE_PAYMENT_STATUS_FAILURE;
    $transaction->message = t('Payment via PayWay Net failed with error code @code.', array('@code' => $_POST['cd_response']));
    commerce_payment_transaction_save($transaction);
    //error_log('Transaction: ' . print_r($transaction,1));
    print('FAILURE NOTIFICATION ACCEPTED');
    drupal_exit();
  }
  drupal_access_denied();
}

/**
 * Handles a user returning back from Payway.
 *
 * Due to the nature of the Payway flow, we do not know if the returning user
 * has had success or failure as Payway displays the return link no matter
 * what the outcome. We therefore avoid the standard checkout/%/payment/return/
 * URL as it will progress the order status if there has been a failure. We
 * leave order status progress up to commerce_payway_net_notify_handler().
 * This page handler just displays a message and forwards the user to the order.
 */
function commerce_payway_net_return_page($order) {
  // If the user does not have access to checkout the order, return a 404. We
  // could return a 403, but then the user would know they've identified a
  // potentially valid checkout URL.
  if (!commerce_checkout_access($order)) {
    return MENU_NOT_FOUND;
  }
  // Check for a completed order status
  if ($order->status == 'completed') {
    drupal_set_message(t('Your order has been successfully processed at Payway.'));
  }
  else {
    drupal_set_message(t("Your order was not fully processed at Payway. It is being reviewed."), 'warning');
  }
  // Redirect to the users version of the order if the user matches the order id.
  global $user;
  if ($order->uid && $user->uid == $order->uid) {
    $order_url = 'user/' . $order->uid . '/orders/' . $order->order_number;
    drupal_goto($order_url);
  }
  else {
    drupal_goto('checkout/' . $order->order_id . '/complete');
  }
}
