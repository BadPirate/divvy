<?hh
require_once('vendor/autoload.php');
require_once('vendor/hh_autoload.php');
require_once('model/model.hh');
require_once('model/event.hh');

use Badpirate\HackTack\Model;

final class GuestModel extends Model {
  public int $id;
  public function __construct(
    public string $name,
    public string $email,
    public string $event_id
  ) {
    $this->id = crc32($email.$event_id);
  }

  static public function create(string $event_id, string $email, string $name) : GuestModel {
    $stmt = parent::prepare(
      'INSERT INTO guests (event_id, email, guest_name) VALUES (?,?,?)'
    );
    $stmt->bind_param('sss',&$event_id, &$email, &$name);
    parent::ec($stmt);
    return new GuestModel(
      $name,
      $email,
      $event_id);
  }

  public function from() : SendGrid\Mail\From {
    return new SendGrid\Mail\From($this->email, $this->name);
  }

  public function to() : SendGrid\Mail\To {
    return new SendGrid\Mail\To($this->email, $this->name);
  }

  static public function forEvent(string $event_id) : Vector<GuestModel> {
    $stmt = parent::prepare(
      'SELECT guest_name, email, event_id
       FROM guests 
       WHERE event_id = ?'
    );
    $stmt->bind_param('s',&$event_id);
    return GuestModel::listFromStmt($stmt);
  }

  static public function listFromStmt(mysqli_stmt $stmt) : Vector<GuestModel> {
    $guest_name = null;
    $guest_email = null;
    $event_id = null;
    $stmt->bind_result(
      &$guest_name,
      &$guest_email,
      &$event_id
    );
    parent::execute($stmt);
    $v = Vector {};
    while($stmt->fetch()) {
      $v[] = new GuestModel(
        $guest_name,
        $guest_email,
        $event_id);
    }
    $stmt->close();
    return $v;
  }
}