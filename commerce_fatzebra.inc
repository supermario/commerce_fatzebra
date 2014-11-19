<?php

/**
 * @file
 * Supporting functions for Fat Zebra payment method for Drupal Commerce.
 */

/**
 * Makes a purchase request and returns a purchase request response object.
 *
 * @param array $request_values
 *   An associative array containing:
 *     - amount: A float value of the purchase amount i.e. 142.33
 *     - reference: A string containing a unique reference for this transaction
 *     - card_holder: A string containing the name of the card holder
 *     - card_number: A string containing the card number
 *     - card_expiry: A string containing the cvv code
 *
 * @return object
 *   An object representing the JSON response received from the Fat Zebra API
 */
function commerce_fatzebra_purchase($request_values, $settings) {
  $gw = commerce_fatzebra_gateway($settings);

  $fraud_data = isset($request_values['fraud']) ? $request_values['fraud'] : null ;

  $purchase_request = new FatZebra\PurchaseRequest(
    $request_values['amount'],
    $request_values['reference'],
    $request_values['card_holder'],
    $request_values['card_number'],
    $request_values['card_expiry'],
    $request_values['cvv'],
    $fraud_data
  );

  $response = null;

  try {
    $response = $gw->purchase($purchase_request);
  } catch (FatZebra\TimeoutException $ex) {
    error_log("Purchase Timeout: Retrying via fetch. Reference: " . $request_values['reference']);
    $response = $gw->get_purchase($request_values['reference']);
    if (!is_null($response)) error_log("Fetch Purchase successful (" . $request_values['reference'] . ")");
  }

  return $response;
}

/**
 *
 * @param  string $token     The FatZebra credit card token
 * @param  int    $amount    A float value of the purchase amount i.e. 142.33
 * @param  string $reference A string containing a unique reference for this transaction
 * @param  string $cvv       The Card CVV code (optional but recommended)
 *
 * @return object
 *   An object representing the JSON response received from the Fat Zebra API
 */
function commerce_fatzebra_token_purchase($token, $amount, $reference, $settings, $cvv = null) {
  $gw = commerce_fatzebra_gateway($settings);

  $response = null;

  try {
    $response = $gw->token_purchase($token, $amount, $reference);
  } catch (FatZebra\TimeoutException $ex) {
    error_log("Token Purchase Timeout: Retrying via fetch. Reference: " . $reference);
    $response = $gw->get_purchase($reference);
    if (!is_null($response)) error_log("Fetch Token Purchase successful (" . $reference . ")");
  }

  return $response;
}

/**
 * Makes a refund request and returns a request response object
 * @param  array $request_values
 *   An associative array containing:
 *     - transaction_id: The Transaction ID of the original transaction to
 *       be refunded (XXX-P-YYYYYY)
 *     - amount: A float value of the purchase amount i.e. 142.33
 *     - reference: A string containing a unique reference for this transaction
 * @param  array $settings       [description]
 * @return object                F
 */
function commerce_fatzebra_refund($request_values, $settings) {
  $gw = commerce_fatzebra_gateway($settings);

  try {
    $response = $gw->refund(
                            $request_values["transaction_id"],
                            $request_values["amount"],
                            $request_values["reference"]
                          );
  } catch (FatZebra\TimeoutException $ex) {
    error_log("Refund Timeout: Retrying via fetch. Reference: " . $request_values["reference"]);
    $response = $gw->get_refund($reference);
    if (!is_null($response)) error_log("Fetch Refund successful (" . $request_values["reference"] . ")");
  }

  return $response;
}

function commerce_fatzebra_gateway($settings) {
  libraries_load('fatzebra_php');

  // For any non-live setting, use true (test mode).
  $transaction_mode = ($settings['txn_mode'] == COMMERCE_FATZEBRA_TXN_MODE_LIVE) ? FALSE : TRUE;

  $gw = new FatZebra\Gateway(
    $settings['username'],
    $settings['token'],
    $transaction_mode,
    $settings['gateway']
  );

  return $gw;
}

/**
 * Processes a Fat Zebra purchase response and create a transaction record.
 *
 * @param object $response
 *   An object representing JSON response from a Fat Zebra API purchase request
 *     - order_id: must be added manually in order to correctly record the
 *       transaction against an order
 *
 * @return true|string
 *   true if the purchase is successful, a string containing an error message
 *   if unsuccessful
 */
function commerce_fatzebra_response_process($response, $order_id, $payment_method, $transaction = null) {

  $process_result = TRUE;

  if (!isset($transaction)) {
    // Create a new transaction object for the current order.
    $transaction = commerce_payment_transaction_new('commerce_fatzebra', $order_id);
  }

  // Set the message that will be recorded for this transaction.
  $transaction->message = 'Transaction @status: @statusdetail.';
  $transaction->message_variables = array();

  // The instance ID identifies which payment method instance was used for this transaction
  $transaction->instance_id = $payment_method['instance_id'];

  // Set the transaction status and text based on response success values.
  if ($response->successful && $response->response->successful) {
    // Our payment was successful.
    $transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
    $transaction->message_variables['@status'] = 'success';
  }
  else {
    // Our payment was unsuccessful, or we received an API error.
    $transaction->status = COMMERCE_PAYMENT_STATUS_FAILURE;
    $transaction->message_variables['@status'] = 'failed';
  }

  if ($response->successful === FALSE) {

    // We have an API error, set an error message to be displayed to the user.
    $messages = implode(', ', $response->errors);
    $transaction->message_variables['@statusdetail'] = $messages;
    $process_result = t("There was a problem communicating with the payment gateway: @messages. Please try again or contact @site_mail for help.", array('@messages' => $messages, '@site_mail' => variable_get('site_mail', '')));
    watchdog('commerce_fatzebra', 'API Error: @response', array('@response' => print_r($response, TRUE)), WATCHDOG_ERROR);

  }
  else {
    // API call was successful, so $response->response will be populated.
    $transaction->message_variables['@statusdetail'] = $response->response->message;

    $amount = $response->response->amount;
    if (strpos($response->response->id, '-R-') !== false) {
      $amount = $amount * -1; // Flip the amount if -R- is in the ID
    }

    $transaction->amount = $amount;
    $transaction->remote_id = $response->response->id;
    $transaction->message .= ' Card: @cardnumber';
    $transaction->message_variables['@cardnumber'] = $response->response->card_number;

    // If the transaction was not successful, set an error message to be
    // displayed to the user.
    if ($response->response->successful === FALSE) {
      $process_result = t("There was a problem with your transaction: @message. Please try again or contact @site_mail for help.", array(
        '@site_mail' => variable_get('site_mail', ''),
        '@message' => $response->response->message)
      );
      watchdog('commerce_fatzebra', 'Transaction Error: @response', array('@response' => print_r($response, TRUE)), WATCHDOG_ERROR);
    }

  }

  commerce_payment_transaction_save($transaction);

  return $process_result;
}

/**
 * Exctract order information for Retail Decisions fraud detection to be added
 * to the FZ transaction payload
 *
 * @param  object $order
 *   Commerce order object
 * @param  array $form
 *   Form values submitted with the purchase
 * @return array
 *   Order data organised into fraud payload according to FZ docs
 *
 * @see https://confluence.fatzebra.com.au/display/DOC/Purchases+with+Retail+Decisions
 */
function commerce_fatzebra_order_red_data($order, $form) {

  global $base_url;
  libraries_load('fatzebra_php');

  $fraud = array(
    'website' => $base_url,
    'device_id' => $form['device_id']
  );

  $order = entity_metadata_wrapper('commerce_order', $order);

  if (isset($order->commerce_customer_billing)) {
    $billing =  $order->commerce_customer_billing;
    $billing_addr =  $billing->commerce_customer_address;

    $fraud['customer'] = array(
      'address_1' => $billing_addr->thoroughfare->value(),
      'city' => $billing_addr->locality->value(),
      'country' => FatZebra\Helpers::iso3166_alpha3($billing_addr->country->value()),
      'email' => $order->mail->value(),
      'first_name' => $billing_addr->first_name->value(),
      'last_name' => $billing_addr->last_name->value(),
      'post_code' => $billing_addr->postal_code->value()
    );
  }

  // User accounts aren't available for first time (anonymous) orders
  $owner_value = $order->owner->value();
  if (!empty($owner_value)) {
    $fraud['customer']['created_at'] = date('c', $order->owner->created->value());
    $fraud['customer']['existing_customer'] = true;
  }

  if (isset($order->commerce_customer_shipping)) {
    $shipping = $order->commerce_customer_shipping;
    $shipping_addr = $shipping->commerce_customer_address;
    $fraud['shipping_address'] = array(
      'address_1' => $shipping_addr->thoroughfare->value(),
      'city' => $shipping_addr->locality->value(),
      'country' => FatZebra\Helpers::iso3166_alpha3($shipping_addr->country->value()),
      'first_name' => $shipping_addr->first_name->value(),
      'last_name' => $shipping_addr->last_name->value(),
      'post_code' => $shipping_addr->postal_code->value(),
      'shipping_method' => 'low_cost'
    );
  }

  foreach ($order->commerce_line_items as $line_item) {
    if($line_item->type->value() == 'product'){
      $fraud['items'][] = array(
        'cost' => $line_item->commerce_unit_price->amount_decimal->value(),
        'description' => $line_item->commerce_product->title->value(),
        'line_total' => $line_item->commerce_total->amount_decimal->value(),
        'product_code' => $line_item->commerce_product->sku->value(),
        'qty' => $line_item->quantity->value(),
        'sku' => $line_item->commerce_product->sku->value()
      );
    }
  }

  return $fraud;
}
