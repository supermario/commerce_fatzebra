(function ($) {
  Drupal.behaviors.commerce_fatzebra = {
    _processed: false,
    _processing: false,

    tokenize: function(submit) {
      // Terminate early if we're already processing a request
      if (this._processing) { return false; }

      // Skip to submit if we're finished
      if (this._processed) { return true; }

      // Start processing a new request
      this._processing = true;

      // Remember the form so we can trigger the submit later
      this._submit = submit;

      v = function(name) { return $('[name="commerce_payment[payment_details][credit_card]['+name+']"]').val(); }
      req = {
        card_holder: v('name'),
        card_number: v('number'),
        expiry_month: v('exp_month'),
        expiry_year: v('exp_year'),
        cvv: v('code'),
        return_path: Drupal.settings.commerce_fatzebra.return_path,
        verification: Drupal.settings.commerce_fatzebra.verification
      };

      var that = this;
      $.ajax({
        type: "GET",
        url: Drupal.settings.commerce_fatzebra.directpost_url,
        data: req,
        jsonpCallback: "Drupal.behaviors.commerce_fatzebra.jsonp",
        contentType: "text/javascript",
        dataType: "jsonp",
        error: function(e) {
          that._processing = false;
          message = "Sorry we encountered an error validating your credit card - please try again.\n\nIf this error persists please contact the website owner.";
          alert(message);
        }
      });

      return this._processed;

    },
    jsonp: function(data) {

      // Disable all inputs within the payment details area, we don't want
      // them sent to the server at all so we say PCI compliant
      $('#payment-details').find('input[type=text],select').attr('disabled',true);

      $('#payment_details_token_status').val(data.r);

      if (data.r == 1) {
        $('#payment_details_token').val(data.token);
      }

      this._processing = false;
      this._processed = true;
      $(this._submit).submit();
    }

  };
})(jQuery);
