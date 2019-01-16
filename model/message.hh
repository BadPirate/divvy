<?hh
require_once('vendor/autoload.php');
require_once('vendor/hh_autoload.php');
require_once('model/model.hh');

use Badpirate\HackTack\Model;

final class MessageModel extends Model
{
  public function __construct(
    public string $event_id,
    public string $guest_email,
    public string $message,
    public string $sender_name
  ) {}

  static public function create(
    string $event_id,
    string $email,
    string $message
  )
  {
    $stmt = parent::prepare(
      'INSERT INTO messages (event_id, email, message) VALUES (?,?,?)'
    );
    $stmt->bind_param('sss',&$event_id, &$email, &$message);
    parent::ec($stmt);
  }

  static public function forEvent(string $event_id) : Vector<MessageModel> {
    $stmt = parent::prepare(
      'SELECT messages.email, messages.message, guests.guest_name
       FROM messages 
       LEFT JOIN guests ON guests.email = messages.email AND guests.event_id = messages.event_id
       WHERE messages.event_id = ? 
       ORDER BY created'
    );
    $stmt->bind_param('s',&$event_id);
    $email = '';
    $message = '';
    $name = '';
    $stmt->bind_result(&$email, &$message, &$name);
    parent::execute($stmt);
    $result = Vector {};
    while($stmt->fetch()) {
      $result[] = new MessageModel(
        $event_id,
        $email,
        $message,
        $name);
    }
    return $result;
  }
}