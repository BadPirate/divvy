<?hh
require_once('vendor/autoload.php');
require_once('vendor/hh_autoload.php');
require_once('model/model.hh');

use Badpirate\HackTack\Model;

class TransactionModel extends Model {
  public function __construct(
    public string $event_id,
    public string $transaction_id,
    public string $description,
    public int $payee_id,
    public Vector<int> $payer_ids,
    public float $amount,
    public bool $payee_paid
  ) {}

  static public function forEvent(string $event_id) : Map <string,TransactionModel> {
    $stmt = parent::prepare(
      'SELECT transaction, description, payer, payee, amount, payee_paid
       FROM transactions
       WHERE event_id = ?
       ORDER BY created'
    );
    $stmt->bind_param('s',&$event_id);
    $tx_id = '';
    $tx_description = '';
    $tx_payer = 0;
    $tx_payee = 0;
    $tx_amount = 0;
    $payee_paid = 1;
    $stmt->bind_result(&$tx_id, &$tx_description, &$tx_payer, &$tx_payee, &$tx_amount, &$payee_paid);
    parent::execute($stmt);
    $txs = Map {};
    while($stmt->fetch()) {
      $tx = $txs->containsKey($tx_id)
        ? $txs[$tx_id]
        : new TransactionModel(
            $event_id,
            $tx_id, 
            $tx_description, 
            $tx_payee, 
            Vector {}, 
            floatval($tx_amount),
            boolval($payee_paid));
      $tx->amount = $tx_amount > $tx->amount ? $tx_amount : $tx->amount;
      $tx->payer_ids[] = $tx_payer;
      $txs[$tx_id] = $tx;
    }
    $stmt->close();
    return $txs;
  }

  static public function delete(string $transaction_id) {
    $stmt = parent::prepare(
      'DELETE FROM transactions WHERE transaction = ?'
    );
    $stmt->bind_param('s',&$transaction_id);
    parent::ec($stmt);
  }

  static public function create(
    string $event_id,
    string $description,
    int $payee_id,
    Vector<int> $payer_ids,
    float $amount
  ) : TransactionModel 
  {
    $transaction_id = parent::generateRandomString(10);
    $stmt = parent::prepare(
      'INSERT INTO transactions (transaction, event_id, description, payee, payer, amount, payee_paid)
       VALUES (?,?,?,?,?,?,?)'
    );
    $payer = 0;
    $payee_paid = $payer_ids->linearSearch($payee_id) !== -1;
    $stmt->bind_param('sssiiii',
      &$transaction_id,
      &$event_id,
      &$description,
      &$payee_id,
      &$payer,
      &$amount,
      &$payee_paid
    );
    $amount = round($amount / count($payer_ids), 2);
    foreach ($payer_ids as $payer_id) {
      $payer = $payer_id;
      if ($payer_id === $payee_id) continue;
      parent::execute($stmt);
    }
    $stmt->close();
    return new TransactionModel(
      $event_id,
      $transaction_id,
      $description,
      $payee_id,
      $payer_ids,
      $amount / count($payer_ids),
      $payee_paid);
  }
}