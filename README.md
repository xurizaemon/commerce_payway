Configuration
-------------

If these instructions need to be updated, please let us know using the
Drupal.org issue queue @ https://drupal.org/project/issues/commerce_payway_net

* Visit https://www.payway.com.au/ and log in using your PayWay credentials.
* In the sidebar menu, choose "Setup Net".
 * Note the "PayWay Biller Code" on this screen, you will need it later.
* Click "Setup".
* Select "Bill Payments/Shopping Cart - I need to pass across information from my website"
* (customise your CSS / logos as you please)
* Under "Browser Return" -
 * "Browser Return URL Pre Payment" - http://example.org/checkout
 * "Browser Return URL" - http://example.org/checkout/commerce_payway_notify
 * "Notification URL" - https://example.org/checkout/payway_net_notify (requires SSL)
 * "Notification Payment Status" - all
 * "Notification Post Type" - extended
* Next!
* The "Security Information" page provides three of the values you need to
  configure your payment method at admin/commerce/config/payment-methods
  * Encryption Key, Username and Password are on the "Security Information" page.
  * "PayWay Biller Code" was the number recorded on the first step of this wizard.
* "Merchant ID" is shown a couple of screens later on "Account
  Settings" page (use "TEST" if you are in test phase).
