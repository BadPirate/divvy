<?hh
require_once('vendor/autoload.php');
require_once('vendor/hh_autoload.php');

require_once('model/event.hh');
require_once('model/guest.hh');
require_once('model/email.hh');
require_once('model/transaction.hh');

$event = EventModel::forId($_REQUEST['id']);
if (!isset($_REQUEST['g'])) {
  print 
    <html><head:jstrap><title>Select guest</title></head:jstrap>
      <body><h5>To see divvy for: '{$event->title}' Select your name.</h5>
        {selectGuest($event,"Guest Name:")}
      </body>
    </html>;
  die();
}
$g = intval($_REQUEST['g']);
if (!$event) throw new Exception("Unknown event!");

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
  if (isset($_REQUEST['action'])) {
    switch ($_REQUEST['action']) {
      case 'Delete':
        TransactionModel::delete($_REQUEST['tx_id']);
        break;
      default: // New transaction
        throw new Exception("Unhandled action ".$_REQUEST['action']);
        break;
    }
  } else {
    $payee = 0;
    $payers = Vector {};
    foreach($_REQUEST['guest_id'] as $key => $guest_id) {
      $guest_id = intval($_REQUEST['guest_id'][$key]);
      if (intval($_REQUEST['paid'][$key]) == 1) $payee = $guest_id;
      if (intval($_REQUEST['divvy'][$key]) == 1) $payers[] = $guest_id;
    }
    if ($payee == 0) throw new Exception("No payee");
    $amount = floatval($_REQUEST['text-amount']);
    if (count($payers) != 0 && $amount > 0) {
      TransactionModel::create(
        $event->id,
        $_REQUEST['text-description'],
        $payee,
        $payers,
        $amount);
    }
  }
}

function guestRow(GuestModel $guest, bool $paid) : :xhp {
  $paid_shared = "btn button-paid";
  $paid_enabled = "$paid_shared btn-success";
  $paid_disabled = "$paid_shared";

  $divvy_shared = "btn divvy-watch col-auto";
  $divvy_enabled = "$divvy_shared btn-primary";
  $divvy_disabled = "$divvy_shared";

  return
    <li class="list-group-item d-flex">
      <input type="hidden" name="guest_id[]" value={"".$guest->id}/>
      <input type="hidden" name="paid[]" value={$paid ? '1' : '0'} 
       class="hidden-paid" id={"hidden-paid-$guest->id"}/>
      <input type="hidden" name="divvy[]" value="1" id={"hidden-divvy-$guest->id"}
       class="hidden-divvy"/>
      <span class="col-auto mr-auto">{$guest->name}</span>
      <div class="btn-group ml-1">
        <button type="button" class={$paid ? $paid_enabled : $paid_disabled}
         id={"button-paid-$guest->id"} onclick={"
           $('.hidden-paid').val('0');
           $('.button-paid').attr('class','$paid_disabled');
           $('#hidden-paid-$guest->id').val('1');
           $('#button-paid-$guest->id').attr('class','$paid_enabled');
         "}>
          Paid
        </button>
      </div>
      <div class="btn-group d-flex col-auto">
        <button type="button" class={$divvy_enabled} id={"button-divvy-$guest->id"}
         onclick={"
          $('#button-exclude-$guest->id').attr('class','$divvy_disabled');
          $('#hidden-divvy-$guest->id').val('1');
          $('#button-divvy-$guest->id').attr('class','$divvy_enabled');
          calculate();
         "}>
          Divvy
        </button>
        <button type="button" class={$divvy_disabled} id={"button-exclude-$guest->id"}
         onclick={"
          $('#button-exclude-$guest->id').attr('class','$divvy_enabled');
          $('#hidden-divvy-$guest->id').val('0');
          $('#button-divvy-$guest->id').attr('class','$divvy_disabled');
          calculate();
         "}>
           Exclude
        </button>
      </div>
      <span class="ml-1 h5 display-divvy col-2">$0.00</span>
    </li>;
}

$coming_xhp = <ul class="list-group"/>;

foreach($event->guests as $guest) {
  $coming_xhp->appendChild(guestRow($guest, $guest->id === $event->primary->id));
}

$guest_xhp = 
  <li class="list-group-item d-flex bg-secondary text-light">
    <span class="col">Transaction</span>
  </li>;

foreach($event->guests as $guest) {
  $guest_xhp->appendChild(
    <span class="col">{$guest->name}</span>
  );
}

$guest_xhp->appendChild(<span class="col"/>); // Button column

$transaction_list_xhp = 
  <ul class="list-group">
    {$guest_xhp}
  </ul>;

$txs = TransactionModel::forEvent($event->id);

function payButton(
  string $description, 
  string $id, 
  string $to, 
  float $amount) : ?:xhp 
{
  return
    <form action="https://www.paypal.com/cgi-bin/webscr" method="post" target="new">
      <input type="hidden" name="cmd" value="_xclick"/>
      <input type="hidden" name="business" value={$to}/>
      <input type="hidden" name="lc" value="US"/>
      <input type="hidden" name="item_name" value={$description}/>
      <input type="hidden" name="item_number" value={$id}/>
      <input type="hidden" name="amount" value={"".$amount}/>
      <input type="hidden" name="currency_code" value="USD"/>
      <input type="hidden" name="button_subtype" value="services"/>
      <input type="hidden" name="no_note" value="0"/>
      <input type="hidden" name="bn" value="PP-BuyNowBF:btn_paynow_SM.gif:NonHostedGuest"/>
      <input type="image" src="https://www.paypalobjects.com/en_US/i/btn/btn_paynow_SM.gif" name="submit" alt="PayPal - The safer, easier way to pay online!"/>
    </form>;
}

if (count($txs)) {
  $tx_totals = Map {};
  foreach($txs as $tx) {
    $transaction_row_xhp = 
      <li class="list-group-item d-flex">
        <span class="col">
          {$tx->description} (${
            $tx->amount * (count($tx->payer_ids) + ($tx->payee_paid ? 1 : 0))
          })</span>
      </li>;
    foreach($event->guests as $guest) {
      $total = 0;
      if ($tx->payer_ids->linearSearch($guest->id) !== -1) {
        $total -= $tx->amount;
      }
      if ($tx->payee_id === $guest->id) {
        $total += $tx->amount * count($tx->payer_ids);
      }
      $transaction_row_xhp->appendChild(
        <span class="col">${round($total,2)}</span>
      );
      $tx_totals[$guest->id] = $tx_totals->containsKey($guest->id) ?  $tx_totals[$guest->id] + $total : $total;
    }
    $transaction_row_xhp->appendChild(
      <form method="post" class="col">
        <input type="hidden" name="tx_id" value={$tx->transaction_id}/>
        <input type="submit" class="btn btn-danger" value="Delete" name="action"/>
      </form>
    );
    $transaction_list_xhp->appendChild($transaction_row_xhp);
  }

  $totals_row_xhp = 
    <li class="list-group-item d-flex bg-info text-white">
      <span class="col">Totals</span>
    </li>;
  
  foreach($event->guests as $guest) {
    $owe = $tx_totals[$guest->id];
    $owed = $tx_totals[$g];
    $payed_button = false;
    $pay = false;
    $guest_me = $g === $guest->id;
    if ($owe == 0) {
      $owes = "";
      $bg = "bg-success";
    } else if ($owe > 0) {
      $owes = "Owed ";
      $bg = "bg-success";
      $pay = true;
    } else {
      $owes = $guest_me ? "I Owe " : "Owes ";
      $bg = "bg-danger";
      $payed_button = !$guest_me && $owed > 0;
    }
    $totals_row_xhp->appendChild(
      <div class={"col $bg text-white"}>
        {$owes} ${abs(round($owe,2))}
        {($tx_totals[$g] < 0 && $pay && !$guest_me)
        ? payButton(
            $event->title,
            $event->id,
            $guest->email,
            $owe < abs($owed) ? $owe : abs($owed))
        : null}
        {$payed_button
         ?  <div>
              <form method="post">
                <input type="hidden" name="guest_id[]" value={$g}/>
                <input type="hidden" name="paid[]" value="0"/>
                <input type="hidden" name="divvy[]" value="1"/>
                <input type="hidden" name="guest_id[]" value={$guest->id}/>
                <input type="hidden" name="paid[]" value="1"/>
                <input type="hidden" name="divvy[]" value="0"/>
                <input type="hidden" name="text-amount" value={abs($owe)}/>
                <input type="hidden" name="text-description" value={
                  "$guest->name paid up"
                }/>
                <input class="btn" type="submit" value="Paid me"/>
              </form>
            </div> : null}</div>
    );
  }
  $transaction_list_xhp->appendChild($totals_row_xhp);

  $transactions_xhp =
    <div class="card mb-3">
      <div class="card-header h6">
        Transactions
      </div>
      <div class="card-body">
        {$transaction_list_xhp}
      </div>
    </div>;
}

foreach ($event->guests as $guest) {
  if ($guest->id === intval($g)) {
    $me = $guest;
  }
}

function selectGuest(EventModel $event, ?string $prompt="Who are you then?") : :xhp {
  $select_guest_name_xhp = 
    <select class="custom-select" onchange={"
      window.location.href='event.hh?id=$event->id&g='+this.value;
    "}>
      <option selected={true}>{$prompt}</option>
    </select>;

  foreach ($event->guests as $guest) {
    $select_guest_name_xhp->appendChild(
      <option value={"".$guest->id}>{$guest->name}</option>
    );
  }
  return $select_guest_name_xhp;
}

$url = getenv('DIVVY_SITE')."/event.hh?id=$event->id";

print 
  <html>
    <head:jstrap reactive={false}>
      <script src="js/divvy.js"></script>
      <title>Divvy {$event->title}!</title>
    </head:jstrap>
    <body class="container" style="width: 970px !important">
      <div>
        <button class="alert alert-info h5 w-100" onclick={"
          var temp = $('<input>');
          $('body').append(temp);
          temp.val('$url').select();
          document.execCommand('copy');
          temp.remove();
          $('#span-share').hide();
          $('#span-copied').show();     
        "}>
          <span id="span-share">Share this trip with attendees: {$url}</span>
          <span id="span-copied" style="display: none;">Copied!</span>
        </button>
      </div>
      <div class="card">
        <div class="card-header">
          <h5>Hi 
            <span id="span-name">
              {$me->name} 
              <small><a href="javascript:
                $('#span-name').hide();
                $('#span-select').show();
              " class="text-small">(Not {$me->name}?)</a></small>
            </span>
            <span id="span-select" class="col-auto mx-3" style="display: none">
              <br/>
              {selectGuest($event)}
            </span>
            Divvy up your trip: {$event->title}
          </h5>
        </div>
        <div class="card-body">
          { count($txs) > 0 ? $transactions_xhp : null }
          <div class="card bg-info">
            <div class="card-header h6 text-white">
              Add an expense
            </div>
            <div class="card-body">
              <form method="post">
                <input type="hidden" name="id" value={$_REQUEST['id']}/>
                <input type="text" class="form-control" placeholder="Describe transaction" 
                 name="text-description" id="text-description" required="1"/>
                <input type="number" class="form-control" placeholder="Amount to split" 
                 name="text-amount" id="text-amount" onchange="calculate();" required="1"/>
                {$coming_xhp}
                <input type="submit" class="btn btn-primary w-100" value="Divvy!"/>
              </form>
            </div>
          </div>
        </div>
      </div>
      <button class="alert alert-info h5 my-3 w-100" onclick="window.location.href='index.hh'">
        Create a new event to Divvy!
      </button>
    </body>
  </html>;