
var calculating = false;
$('.divvy_watch').click(function() {
  calculate();
});

function calculate() {
  if (calculating == true) return;
  calculating = true;
  var amount = parseFloat($('#text-amount').val());
  var divvys_hidden = $('.hidden-divvy');
  var split_ways = 0;
  for(var k in divvys_hidden) {
    var divvy = divvys_hidden.eq(k);
    if (divvy.val()) {
      split_ways += parseInt(divvy.val());
    }
  }
  var divvys_display = $('.display-divvy');
  divvys_display.html('$0.00');
  if (split_ways > 0 && amount > 0) {
    var share = (amount / split_ways).toFixed(2);
    for (var k in divvys_hidden) {
      var hidden_divvy = divvys_hidden.eq(k);
      if (hidden_divvy.val() > 0) {
        var display_divvy = divvys_display.eq(k);
        display_divvy.html("$"+share);
      }
    }
  }
  calculating = false;
}