
This module adds the Australian payment gateway Fat Zebra as a payment method
for Drupal Commerce.

This module is sponsored by Fat Zebra (https://fatzebra.com.au).


INSTALLATION
------------

- Ensure cURL (http://php.net/manual/en/book.curl.php) is installed
- Download the FatZebra PHP Library into sites/all/libraries/fatzebra_php or
  sites/<domain>/libraries/fatzebra_php
    (https://github.com/fatzebra/PHP-Library/zipball/master)
- Enable the Commerce Fat Zebra module and its dependencies
- Enable the payment method under Store > Configuration > Payment methods
- Edit the Payment method's action to configure payment settings, and to set
  username/token if you have a Fat Zebra account
- Ensure your Default store currency is AUD under Store > Configuration >
  Currency settings. The Commerce Fat Zebra payment method will not show for
  non-AUD currency orders.


TESTING
-------

Nothing further is required to fully test the module against the sandbox
environment (module defaults to test credentials), however you will not have
access to the Fat Zebra dashboard to see transaction summaries.

See http://docs.fatzebra.com.au/ for test credit card numbers.

If you'd like to get a Fat Zebra account you can sign up
(https://www.fatzebra.com.au/register/solo) for free to get started.

PCI DDS Compliance
------------------

Provides an optional PCI Compliance Mode, where credit card data is tokenised
via direct post to FatZebra before payment is submitted. This avoids handling of
any credit card data, even temporarily, by the server.

Currently available in 7.x-1.x-dev.

PLANNED FEATURES
--------
* Support for refunds (available in 7.x-1.x-dev)
* Support for Fat Zebra card tokenization using Commerce Card on File to allow
  users to make additional purchases without having to re-enter card data
* Support for recurring payments

MORE INFO
---------

Please see the commerce_fatzebra project page on Drupal.org for more info
(http://drupal.org/project/commerce_fatzebra)
