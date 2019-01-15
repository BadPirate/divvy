<?hh
require_once('vendor/autoload.php');
require_once('vendor/hh_autoload.php');

require_once('model/event.hh');
require_once('utils.hh');

enum MailType : int {
  Created = 1;
  Message = 2;
  Add     = 3;
} 

use SendGrid\Mail\To;
use SendGrid\Mail\Personalization;
use SendGrid\Mail\From;

final class MailModel extends SendGrid\Mail\Mail {
  public function __construct(
    public EventModel $event,
    ?GuestModel $sender = null) 
  {
    $sender = $sender ?? $event->primary;
    parent::__construct(
      new From('divvy@logichigh.com', 'Divvy'),
      null, // to
      null, // subject
      null, // plain text
      null, // html
      [ 'title' => $event->title,
        'site' => getenv('DIVVY_SITE'),
        'sender' => $sender->name,
        'eventid' => $event->id]);
    $this->addSubstitution('title',$event->title);
    $this->addSubstitution('site',getenv('DIVVY_SITE'));
    $this->addSubstitution('sender',$sender->name);
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
    MailModel::sendCheck($this);
  }
  
  static public function sendCheck($mail) {
    $sg = MailModel::sendGrid();
    $response = $sg->send($mail);
    switch($response->statusCode()) {
      case 202: break; // success
      default:
        d($response,$mail);
        die();
        throw new Exception("Unhandled code ".$response->statusCode());
        die();
    }
  }

  public function toAll() {
    foreach($this->event->guests as $guest) {
      $this->to($guest);
    }
  }

  public function to(GuestModel $guest) {
    $p = new To(
      $guest->email,
      $guest->name,
      ['guestid' => $guest->id,
       'guestname' => $guest->name]
    );
    $this->addTo($p);
  }

  static public function sendGeneric(
    string $to_email,
    string $to_name,
    string $subject,
    string $message,
    string $cta,
    string $cta_url
  ) {
    $mail = new SendGrid\Mail\Mail(
      new From('divvy@logichigh.com', 'Divvy'),
      new To($to_email, $to_name),
      $subject,
      null, // plain text
      null, // html
      [ 'message' => $message,
        'site' => getenv('DIVVY_SITE'),
        'cta' => $cta,
        'ctaurl' => $cta_url,
        'subject' => $subject]
    );
    $mail->setTemplateId('d-4435b22427ce4395a6b3095d81ad88c5');
    MailModel::sendCheck($mail);
    d($mail);
  }

  static public function sendTemplate(
    EventModel $event, 
    MailType $template, 
    ?GuestModel $sender = null,
    ?string $message = null,
    ?GuestModel $target = null) 
  {
    switch ($template) {
      case MailType::Created:
        foreach($event->guests as $guest) {
          $e = new MailModel($event);
          $primary = $event->primary;
          $e->setTemplateId('d-4dca1fadb9634247b8ab8ea5fe75edba');
          $e->setAsm(10432);
          $e->to($guest);
          $e->send();
        }
        break;
      case MailType::Message:
        foreach($event->guests as $guest) {
          $e = new MailModel($event,$sender);
          $e->setTemplateId('d-3dbbb3fec2ee47e4a6a638630ac81509');
          $e->setAsm(10445);
          $e->to($guest);
          $e->addSubstitution('message',$message);
          $e->send();
        }
        break;
      case MailType::Add:
        $e = new MailModel($event,$sender);
        $e->setTemplateId('d-3ed8d999a9da437b9b5343e27b9528af');
        $e->setAsm(10432);
        if (!$target) throw new Exception("Target required");
        $e->to($target);
        $e->send();
        break;
    }
  }
}