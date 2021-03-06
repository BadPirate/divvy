<?hh
require_once('vendor/autoload.php');
require_once('vendor/hh_autoload.php');
require_once('template.hh');

require_once('model/event.hh');
require_once('model/guest.hh');
require_once('model/email.hh');
require_once('model/transaction.hh');
require_once('model/message.hh');
require_once('model/user.hh');

use Badpirate\HackTack\HT;

$event = EventModel::forId($_REQUEST['id']);
$g = isset($_REQUEST['g']) ? intval($_REQUEST['g']) : null;

$guest_names = [];

$me = null;
$user = UserModel::fromSession();
foreach ($event->guests as $guest) {
  if ($user && $user->email === $guest->email) {
    if (!$g || $guest->id !== $g) {
      HT::redirect("event.hh?id=$event->id&g=$guest->id");
    }
  }
  if ($guest->id === intval($g)) {
    $me = $guest;
    $event->current = $guest;
  }
}

if (!$me) {
  print 
    <html><head:jstrap><title>Select guest</title></head:jstrap>
      <body><h5>To see divvy for: '{$event->title}' Select your name.</h5>
        {selectGuest($event,"Guest Name:")}
      </body>
    </html>;
  die();
}

if (!$event) throw new Exception("Unknown event!");

if ($user && $me->email !== $user->email) {
  $user->logout("event.hh?id=$event->id&g=$g");
}

if (!$user && UserModel::existsForEmail($me->email)) {
  UserModel::loginRedirect($me->email, "event.hh?id=$event->id&g=$g");
}

if (isset($_REQUEST['action'])) {
  switch ($_REQUEST['action']) {
    case 'delete':
      $event->delete();
      break;
    case 'X':
      TransactionModel::delete($_REQUEST['tx_id']);
      break;
    case 'Message':
      MessageModel::create($event->id,$me->email,$_REQUEST['message']);
      MailModel::sendTemplate($event,MailType::Message,$me,$_REQUEST['message']);
      break;
    case 'add-guest':
      $guest = $event->addGuest($_REQUEST['email'], $_REQUEST['name']);
      MailModel::sendTemplate($event,MailType::Add,$me,null,$guest);
      break;
    default: // New transaction
      throw new Exception("Unhandled action ".$_REQUEST['action']);
      break;
  }
} else if($_SERVER['REQUEST_METHOD'] === 'POST') {
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

function guestRow(GuestModel $guest, bool $paid) : :xhp {
  $paid_shared = "btn button-paid";
  $paid_enabled = "$paid_shared btn-success";
  $paid_disabled = "$paid_shared";

  $divvy_shared = "btn divvy-watch";
  $divvy_enabled = "$divvy_shared btn-primary";
  $divvy_disabled = "$divvy_shared";

  return
    <div class="bg-white mt-1 p-1">
      <input type="hidden" name="guest_id[]" value={"".$guest->id}/>
      <input type="hidden" name="paid[]" value={$paid ? '1' : '0'} 
       class="hidden-paid" id={"hidden-paid-$guest->id"}/>
      <input type="hidden" name="divvy[]" value="1" id={"hidden-divvy-$guest->id"}
       class="hidden-divvy"/>
      <div class="h5">
        {$guest->name}
        <span class="ml-1 h5 display-divvy col-2">$0.00</span>
      </div>
      <div class="row m-0">
        <div class="btn-group mr-auto">
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
        <div class="btn-group">
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
      </div>
    </div>;
}

$coming_xhp = <div/>;

foreach($event->guests as $guest) {
  $coming_xhp->appendChild(guestRow($guest, $guest->id === $event->primary->id));
  $guest_names[] = $guest->name;
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
  <ul class="list-group d-none d-md-block">
    {$guest_xhp}
  </ul>;

$transaction_list_compact_rows = <tbody/>;


$transaction_list_compact_xhp = 
  <table class="table table-striped d-md-none">
    {$transaction_list_compact_rows}
  </table>;

$transaction_list_compact_summary_xhp = <div class="d-md-none"/>;

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
      <input type="image" src="https://www.paypalobjects.com/en_US/i/btn/btn_paynow_SM.gif" name="submit" 
       alt="PayPal - The safer, easier way to pay online!"/>
    </form>;
}

if (count($txs)) {
  $tx_totals = Map {};
  foreach($txs as $tx) {
    $transaction_row_xhp = 
      <li class="list-group-item d-flex">
        <span class="col">
          {$tx->description} (${ $tx->total() })</span>
      </li>;
    $payer_names = [];
    foreach($event->guests as $guest) {
      $total = 0;
      $is_payee = $tx->payee_id === $guest->id;
      if ($tx->payer_ids->linearSearch($guest->id) !== -1) {
        $total -= $tx->amount;
        $payer_names[] = $guest->name;
      }
      if ($is_payee) {
        if ($tx->payee_paid) {
          $total -= $tx->amount;
          $payer_names[] = $guest->name;
        }
        $payee_name = $guest->name;
      }
      $transaction_row_xhp->appendChild(
        <span class="col">
          { $is_payee
            ? <span class="bg-success text-light" style="display: block;">Paid ${round($tx->total())}</span> : null }
          <span>${round($total,2)}</span>
        </span>
      );
      if ($is_payee) $total += $tx->total();
      $tx_totals[$guest->id] = 
        $tx_totals->containsKey($guest->id) 
        ? $tx_totals[$guest->id] + $total 
        : $total;
    }
    $transaction_list_compact_rows->appendChild(
      <tr>
        <td>
          <form method="post" class="col">
            <input type="hidden" name="tx_id" value={$tx->transaction_id}/>
            <input type="submit" class="btn btn-danger float-right" value="X" name="action"/>
          </form>
          {count($payer_names) > 1
           ? implode(', ',$payer_names)." split the cost ($".$tx->total().") of $tx->description.  $payee_name paid."
           : "$payee_name paid ".$payer_names[0]." $$tx->amount."}
        </td>
      </tr>
    );
    $transaction_row_xhp->appendChild(
      <form method="post" class="col">
        <input type="hidden" name="tx_id" value={$tx->transaction_id}/>
        <input type="submit" class="btn btn-danger" value="X" name="action"/>
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
    $payed_button_xhp = $payed_button ?
      <div>
        <form method="post" class="d-flex">
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
          <input class="btn col-12" type="submit" value="Paid me"/>
        </form>
      </div> : null;

    $pay_button_xhp = 
      ($tx_totals[$g] < 0 && $pay && !$guest_me)
      ? payButton(
          $event->title,
          $event->id,
          $guest->email,
          $owe < abs($owed) ? $owe : abs($owed))
      : null;

    $transaction_list_compact_summary_xhp->appendChild(
      <div class={"$bg text-light p-3"}>
        <span class="mr-auto">
          {"$guest->name $owes $".abs(round($owe,2))}
        </span>
        { $pay_button_xhp }
        { $payed_button_xhp }
      </div>
    );

    $totals_row_xhp->appendChild(
      <div class={"col $bg text-white"}>
        {$owes} ${abs(round($owe,2))}
        {$pay_button_xhp}
        { $payed_button_xhp }</div>
    );
  }
  $totals_row_xhp->appendChild(<span class="col"/>);
  $transaction_list_xhp->appendChild($totals_row_xhp);

  $transactions_xhp =
    <div class="card mb-3">
      <div class="card-header h6">
        Transactions
      </div>
      <div class="card-body p-0 p-md-3">
        {$transaction_list_xhp}
        {$transaction_list_compact_xhp}
        {$transaction_list_compact_summary_xhp}
      </div>
    </div>;
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

$messages_xhp = <tbody/>;

foreach(MessageModel::forEvent($event->id) as $message) {
  $messages_xhp->appendChild(
    <tr>
      <td><span>{"$message->sender_name: $message->message"}</span></td>
    </tr>
  );
}

print 
  <html>
    <head:divvy>
      <script src="js/divvy.js"></script>
      <title>Divvy {$event->title}!</title>
    </head:divvy>
    <body>
      <divvy:nav event={$event}/>
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
      <div class="alert alert-info p-0 p-md-3">
        <div class="m-3 m-md-0" id="div-guestlist">
          Who's in?  {implode(', ',$guest_names)} 
          <button class="fas fa-user-plus ml-3 btn btn-primary" onclick="
            $('#div-guestlist').hide();
            $('#div-addguest').show();
          "/>
        </div>
        <form method="post" class="input-group m-0" id="div-addguest" style="display: none;">
          <input type="text" name="name" class="form-control" placeholder="Guest Name" required={true}/>
          <input type="email" name="email" class="form-control" placeholder="Guest Email" required={true}/>
          <div class="input-group-append">
            <div class="input-group-text">
              <input type="hidden" name="action" value="add-guest"/>
              <button type="submit" class="fas fa-user-plus btn btn-primary"/>
            </div>
          </div>
        </form>
      </div>
      <div class="card mb-3 pb-1">
        <div class="card-header h5">
          Messages
        </div>
        <div class="card-body px-0 px-md-3 pb-0">
          <table class="table table-striped m-0">
            {$messages_xhp}
          </table>
          <form method="post" class="input-group">
            <input type="text" required={true} placeholder="Send a message to group" 
              name="message" class="form-control"/>
            <div class="input-group-append">
              <input type="submit" value="Message" name="action"/>
            </div>
          </form>
        </div>
      </div>
      { count($txs) > 0 ? $transactions_xhp : null }
      <div class="card bg-info">
        <div class="card-header h6 text-white">
          Add an expense
        </div>
        <div class="card-body p-0 pt-3 p-md-3">
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
      <button class="alert alert-info h5 my-3 w-100" onclick="window.location.href='index.hh'">
        Create a new event to Divvy!
      </button>
      <button class="alert alert-danger h5 my-3 w-100" 
       onclick={"
        if(confirm('Are you sure, this will delete all the messages and transactions associated with this event as well...')) {
          window.location.href='event.hh?id=$event->id&g=$g&action=delete';
        }
       "}>DELETE Event</button>
    </body>
  </html>;