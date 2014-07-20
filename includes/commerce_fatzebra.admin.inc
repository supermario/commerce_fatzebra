<?php

/**
 * @file
 * Administrative forms for the FatZebra module.
 */

/**
 * Form callback: allows the user to void a transaction.
 */
function commerce_fatzebra_refund_form($form, &$form_state, $order, $transaction) {
  $form_state['transaction'] = $transaction;

  // Load and store the payment method instance for this transaction
  $payment_method = commerce_payment_method_instance_load($transaction->instance_id);
  $form_state['payment_method'] = $payment_method;

  $form['markup'] = array(
    '#markup' => t('Are you sure that you want to refund this transaction?'),
  );

  $form = confirm_form(
    $form,
    t('Are you sure that you want to refund this transaction?'),
    'admin/commerce/orders/' . $order->order_id . '/payment',
    '',
    t('Refund'),
    t('Cancel'),
    'confirm'
  );

  return $form;
}

/**
 * Submit handler: process the void request
 */
function commerce_fatzebra_refund_form_submit($form, &$form_state) {
  $transaction = $form_state['transaction'];

  // Payment method not loading for some reason...?
  $payment_method = (array)$form_state['payment_method'];
  $payment_method['settings'] = commerce_fatzebra_settings_form_default_values();

  $request_values = array(
    "transaction_id" => $transaction->remote_id,
    "amount" => commerce_currency_amount_to_decimal($transaction->amount, $transaction->currency_code),
    "reference" => $transaction->order_id
  );

  // Submit the request to FatZebra.
  module_load_include('inc', 'commerce_fatzebra', 'commerce_fatzebra');
  $response = commerce_fatzebra_refund($request_values, $payment_method['settings']);

  // Create a new transaction for the refund
  $process_result = commerce_fatzebra_response_process($response, $transaction->order_id);

  // Pass on errors to user if there are any
  if ($process_result !== TRUE) {
    drupal_set_message(check_plain($process_result), 'error');
    return FALSE;
  }
}
