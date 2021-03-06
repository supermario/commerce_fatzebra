<?php

/**
 * @file
 * Administrative forms for the FatZebra module.
 */

/**
 * Form callback: allows the user to void a transaction.
 * @param  $form
 * @param  $form_state
 * @param  $order
 * @param  $transaction
 * @return $form        The Drupal FormAPI form definition
 */
function commerce_fatzebra_refund_form($form, &$form_state, $order, $transaction) {
  $form_state['transaction'] = $transaction;
  $original_amount = commerce_currency_amount_to_decimal($transaction->amount, $transaction->currency_code);
  $remaining_amount = 0;
  foreach(commerce_payment_transaction_load_multiple(array(), array('order_id' => $order->order_id)) as $t) {
    $remaining_amount += $t->amount;
  }
  $remaining_amount = commerce_currency_amount_to_decimal($remaining_amount, $transaction->currency_code);

  // Load and store the payment method instance for this transaction
  $payment_method = commerce_payment_method_instance_load($transaction->instance_id);
  $form_state['order'] = $order;
  $form_state['payment_method'] = $payment_method;

  $form['markup_pre'] = array(
    '#markup' => t('You are refunding from Order ID %order, transaction ID %trans, original transaction amount $%amount, refundable amount $%refundable.',
      array(
        '%order' => $transaction->order_id,
        '%trans' => $transaction->transaction_id,
        '%amount' => $original_amount,
        '%refundable' => $remaining_amount
      )
    ),
  );

  $form['refund_amount'] = array(
    '#type' => 'textfield',
    '#title' => t('Refund amount'),
    '#field_prefix' => '$',
    '#description' => t('The transaction amount in dollars. I.e. $12.34'),
    '#default_value' => $remaining_amount,
    '#required' => true,
  );
  $form['markup_post'] = array(
    '#markup' => t('Are you sure that you want to refund this transaction?'),
  );

  $form = confirm_form(
    $form,
    t('Transaction refund'),
    'admin/commerce/orders/' . $transaction->order_id . '/payment',
    '',
    t('Confirm'),
    t('Cancel'),
    'confirm'
  );

  return $form;
}

/**
 * Form submit handler for transaction refund.
 *
 * @param  $form
 * @param  $form_state
 * @return boolean      Whether refund was error-free
 */
function commerce_fatzebra_refund_form_submit($form, &$form_state) {
  $transaction = $form_state['transaction'];

  $payment_method = (array)$form_state['payment_method'];
  // Merge settings form default values in case user has never saved settings.
  $payment_method['settings'] = (array) $payment_method['settings'] + commerce_fatzebra_settings_form_default_values();

  $request_values = array(
    "transaction_id" => $transaction->remote_id,
    "amount" => $form_state['values']['refund_amount'],
    // Suffix normal order_id reference with -R for refund, and a timestamp for uniqueness
    "reference" => $transaction->order_id . "-R" . time()
  );

  // Submit the request to FatZebra.
  module_load_include('inc', 'commerce_fatzebra', 'commerce_fatzebra');
  $response = commerce_fatzebra_refund($request_values, $payment_method['settings']);

  // Create a new transaction for the refund
  $process_result = commerce_fatzebra_response_process($response, $transaction->order_id, $payment_method);

  // Send user back to the payment form for this order
  $form_state['redirect'] = 'admin/commerce/orders/' . $transaction->order_id . '/payment';

  // Pass on errors to user if there are any
  if ($process_result !== TRUE) {
    drupal_set_message(check_plain($process_result), 'error');
    return FALSE;
  } else {
    drupal_set_message(t('Refund sucessfully processed.'), 'status');
  }
}
