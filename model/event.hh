<?hh
require_once('vendor/autoload.php');
require_once('vendor/hh_autoload.php');
require_once('model/model.hh');
require_once('model/guest.hh');

use Badpirate\HackTack\Model;

final class EventModel extends Model {
  public GuestModel $primary;
  public function __construct(
    public string $id,
    public string $title,
    public Vector<GuestModel> $guests
  ) 
  {
    $this->primary = $guests[0];
  }

  public function url() : string {
    return getenv('DIVVY_SITE')."/event.hh?id=$this->id";
  }

  public function addGuest(string $guest_email, string $guest_name) : GuestModel {
    $g = GuestModel::create($this->id, $guest_email, $guest_name);
    $this->guests[] = $g;
    return $g;
  }

  static private string $all_params = "id, title";

  static public function forId(string $id) : ?EventModel {
    $ap = EventModel::$all_params;
    $stmt = parent::prepare(
      "SELECT $ap FROM events WHERE id = ?"
    );
    $stmt->bind_param('s',&$id);
    $r = EventModel::listFromStmt($stmt);
    return count($r) ? $r[0] : null;
  }

  static public function listFromStmt(mysqli_stmt $stmt) : Vector<EventModel> {
    $event_id = null;
    $event_title = null;
    $stmt->bind_result(
      &$event_id,
      &$event_title
    );
    parent::execute($stmt);
    $event_ids = Map {};
    while($stmt->fetch()) {
      $event_ids[$event_id] = $event_title;
    }
    $stmt->close();
    $events = Vector {};
    foreach($event_ids as $event_id => $event_title) {
      $guests = GuestModel::forEvent($event_id);
      $events[] = new EventModel(
        $event_id,
        $event_title,
        $guests);
    }
    return $events;
  }

  static public function create(
    string $title, 
    string $primary_email, 
    string $primary_name,
    array $other_email_array,
    array $other_name_array) : EventModel {
    $stmt = parent::prepare(
      "INSERT INTO events (id,title) VALUES (?,?)"
    );
    $event_id = parent::generateRandomString(8);
    $stmt->bind_param('ss',&$event_id,&$title);
    parent::ec($stmt);

    $guests = Vector {};
    $guests[] = GuestModel::create($event_id,$primary_email,$primary_name);
    $combined = array_combine($other_email_array,$other_name_array);
    foreach($combined as $email => $name) {
      $guests[] = GuestModel::create($event_id, $email, $name);
    }
    return new EventModel(
      $event_id,
      $title,
      $guests);
  }
}