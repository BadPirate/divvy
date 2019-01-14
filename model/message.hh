<?hh
require_once('vendor/autoload.php');
require_once('vendor/hh_autoload.php');
require_once('model/model.hh');

use Badpirate\HackTack\Model;

final class MessageModel extends Model
{
  public function __construct(
    public string $event_id,
    public int $guest_id,
    public string $message,
    public string $sender_name
  ) {}

  static public function create(
    string $event_id,
    int $guest_id,
    string $message
  )
  {
    $stmt = parent::prepare(
      'INSERT INTO messages (event_id, guest_id, message) VALUES (?,?,?)'
    );
    $stmt->bind_param('sis',&$event_id, &$guest_id, &$message);
    parent::ec($stmt);
  }

  static public function forEvent(string $event_id) : Vector<MessageModel> {
    $stmt = parent::prepare(
      'SELECT messages.guest_id, messages.message, guests.guest_name
       FROM messages 
       LEFT JOIN guests ON guests.id = messages.guest_id
       WHERE messages.event_id = ? 
       ORDER BY created'
    );
    $stmt->bind_param('s',&$event_id);
    $guest_id = 0;
    $message = '';
    $name = '';
    $stmt->bind_result(&$guest_id, &$message, &$name);
    parent::execute($stmt);
    $result = Vector {};
    while($stmt->fetch()) {
      $result[] = new MessageModel(
        $event_id,
        $guest_id,
        $message,
        $name);
    }
    return $result;
  }
}