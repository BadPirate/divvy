<?hh
require_once('vendor/autoload.php');
require_once('vendor/hh_autoload.php');

require_once('model/event.hh');

enum MailType : int {
  Created = 1;
} 


final class MailModel extends SendGrid\Mail\Mail {
  public function __construct(public EventModel $event) {
    parent::__construct();
  }

  public function addXHP(:xhp $xhp) {
    $this->addContent('text/html',$xhp->toString());
  }

  static private ?\SendGrid $send_grid;
  static public function sendGrid() : \SendGrid {
    if (!MailModel::$send_grid) {
      !MailModel::$send_grid = new \SendGrid('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');
    }
    return MailModel::$send_grid;
  }

  public function send() {
    $sg = MailModel::sendGrid();
    $response = $sg->send($this);
  }

  public function toAll() {
    $ev = $this->event;
    foreach($ev->guests as $other) {
      $this->addTo($other->email, $other->name);
    }
  }

  static public function sendTemplate(EventModel $event, MailType $template) {
    switch ($template) {
      case MailType::Created:
        $e = new MailModel($event);
        $primary = $event->primary;
        $e->setFrom($primary->email, $primary->name);
        $e->setSubject("Divvy up expenses for '$event->title'");
        $e->toAll();
        $content = 
          "$primary->name would like to divvy up expenses for your upcomming trip '$event->title'".
          " to add expenses or view current details go to ".$event->url();
        $e->addContent(
          "text/plain", 
          $content);
        $e->addXHP(
          <div>
            <h1>Divvy!</h1>
            <p>{$content}</p>
            <p><a href={$event->url()}>View {$event->title}</a></p>
          </div>
        );
        $e->send();
        break;
    }
  }
}