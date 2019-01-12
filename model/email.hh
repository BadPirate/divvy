<?hh
require_once('vendor/autoload.php');
require_once('vendor/hh_autoload.php');

require_once('model/event.hh');
require_once('utils.hh');

enum MailType : int {
  Created = 1;
} 

use SendGrid\Mail\To;
use SendGrid\Mail\Personalization;
use SendGrid\Mail\From;

final class MailModel extends SendGrid\Mail\Mail {
  public function __construct(public EventModel $event) {
    parent::__construct(
      $event->primary->from(),
      null, // to
      null, // subject
      null, // plain text
      null, // html
      [ 'title' => $event->title,
        'site' => getenv('DIVVY_SITE'),
        'sender' => $event->primary->name,
        'eventid' => $event->id]);
    $this->addSubstitution('title',$event->title);
    $this->addSubstitution('site',getenv('DIVVY_SITE'));
    $this->addSubstitution('sender',$event->primary->name);
    $this->addSubstitution('eventid',$event->id);
  }

  public function addXHP(:xhp $xhp) {
    $this->addContent('text/html',$xhp->toString());
  }

  static private ?\SendGrid $send_grid;
  static public function sendGrid() : \SendGrid {
    if (!MailModel::$send_grid) {
      !MailModel::$send_grid = new \SendGrid(getenv('SENDGRID_API_KEY'));
    }
    return MailModel::$send_grid;
  }

  public function send() {
    $sg = MailModel::sendGrid();
    $response = $sg->send($this);
    switch($response->statusCode()) {
      case 202: break; // success
      default:
        throw new Exception($response);
        die();
    }
  }

  public function toAll() {
    $ev = $this->event;
    foreach($ev->guests as $guest) {
      $this->addTo($guest->to());
    }
  }

  static public function sendTemplate(EventModel $event, MailType $template) {
    switch ($template) {
      case MailType::Created:
        $e = new MailModel($event);
        $primary = $event->primary;
        $e->setFrom($primary->email, $primary->name);
        $e->setTemplateId('d-4dca1fadb9634247b8ab8ea5fe75edba');
        $e->setAsm(10432);
        foreach($event->guests as $guest) {
          $p = new To(
            $guest->email,
            $guest->name,
            ['guestid' => $guest->id]
          );
          $e->addTo($p);
        }
        $e->send();
        break;
    }
  }
}