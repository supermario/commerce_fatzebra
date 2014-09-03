(function ($) {
  Drupal.behaviors.commerce_fatzebra = {
    _processed: false,

    tokenize: function(submit) {
      // Remember the form so we can trigger the submit later
      this._submit = submit;

      // @STUB Need to pull real data from form.
      req = {
        card_holder: 'Jim Citizen',
        card_number: '4005 5500 0000 0001',
        expiry_month: '9',
        expiry_year: '2019',
        cvv: '878',
        return_path: 'https://gateway.sandbox.fatzebra.com.au',
        verification: '0dd46be9107ac9c4483302f156a642cd'
      };

      $.ajax({
        type: "GET",
        url: "https://gateway.sandbox.fatzebra.com.au/v2/credit_cards/direct/TEST.json",
        data: req,
        jsonpCallback: "Drupal.behaviors.commerce_fatzebra.jsonp",
        contentType: "text/javascript",
        dataType: "jsonp",
        error: function(e) {
          // Handle errors here - all non HTTP 2xx errors, such as HTTP 500, 403, 401 etc
          // (very unlikely, but HTTP 500 or HTTP 502 is the most likely to happen)
          alert(e.message);
        }
      });

      return this._processed;

    },
    jsonp: function(data) {

      $('#payment_details_token_status').val(data.r);

      this._processed = true;

      if (data.r == 1) {
        $('#payment_details_token').val(data.token);
      }

      $(this._submit).submit();
    }

  };
})(jQuery);
