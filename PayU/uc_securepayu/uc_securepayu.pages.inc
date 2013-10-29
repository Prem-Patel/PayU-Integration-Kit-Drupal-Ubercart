<?php

require_once("Rc43.inc");

function uc_securepayu_complete($cart_id = 0) {
  watchdog('securepayu', 'Receiving new order notification for order !order_id.', array('!order_id' => check_plain($_SESSION['cart_order'])));
  $key = $_POST['key'];
  $salt = variable_get('uc_securepayu_secret_salt', '');
  $txnid = $_POST['txnid'];
  $firstname = $_POST['firstname'];
  $productinfo = $_POST['productinfo'];
  $amount = $_POST['amount'];
  $email = $_POST['email'];
  $status = $_POST['status'];
  $string = $salt . '|' . $status . '|||||||||||' . $email . '|' . $firstname . '|' . $productinfo . '|' . $amount . '|' . $txnid . '|' . $key;
  $hash = strtolower(hash('sha512', $string));
  $response_hash = $_POST['hash'];
  $order = uc_order_load($_POST['txnid']);

  if ($order === FALSE) {
    return t('An error has occurred. Please contact us to ensure your order has submitted.');
  }
  $status = $_POST['status'];
  $amount = $_POST['amount'];
  $payment_id = $_POST['mihpayid'];
  $context = array(
      'revision' => 'formatted-original',
      'type' => 'amount',
  );
  $options = array(
      'sign' => FALSE,
  );
  if ($status == 'success') {
    if ($hash == $response_hash) {
      $comment = t('Secure payu Payment ID: @payment_id', array('@payment_id' => $mihpayid));
      uc_payment_enter($order->order_id, 'securepayu', $amount, $order->uid, NULL, $comment);
      uc_cart_complete_sale($order);
      uc_order_comment_save($order->order_id, 0, t('Payment of @amount submitted through payu.', array('@amount' => uc_price($amount, $context, $options))), 'order', 'payment_received');
      $order->order_status = 'Completed';
    }
  } else {
    uc_order_comment_save($order->order_id, 0, t("The customer's attempted payment from a bank account failed."), 'admin');
    $order->order_status = 'Canceled';
    drupal_set_message(t('Your payu payment was cancelled. Please feel free to continue shopping or contact us for assistance.'));
  }
  uc_order_save($order);
  // Add a comment to let sales team know this came in through the site.
  uc_order_comment_save($order->order_id, 0, t('Order created through wpayuite.'), 'admin');

  if ($order->order_status == 'Canceled') {
    drupal_goto('cart');
  } else {
    // Empty that cart...
    uc_cart_empty($cart_id);
    $output = uc_cart_complete_sale($order, variable_get('uc_new_customer_login', FALSE));
    $page = variable_get('uc_cart_checkout_complete_page', '');
  }

  if (!empty($page)) {
    drupal_goto($page);
  }

  return $output;
}
