<?hh
require_once('vendor/autoload.php');
require_once('vendor/hh_autoload.php');
require_once('template.hh');

require_once('model/event.hh');
require_once('model/email.hh');
require_once('model/user.hh');

use Badpirate\HackTack\HT;

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
  $e = EventModel::create(
    $_REQUEST['title'],
    $_REQUEST['primary-email'],
    $_REQUEST['primary-name'],
    $_REQUEST['email'],
    $_REQUEST['name']);
  MailModel::sendTemplate($e,MailType::Created);
  HT::redirect("event.hh?id=$e->id&g=".$e->primary->id);
}

$user = UserModel::fromSession();
$events_list_xhp = null;
if ($user) {
  $events = EventModel::forEmail($user->email);
  if (count($events)) {
    $events_list_xhp = <table class="table table-striped"/>;
    foreach($events as $event) {
      $events_list_xhp->appendChild(
        <tr><td><a href={"event.hh?id=$event->id"}>{$event->title}</a></td></tr>
      );
    }
  }
}


print
<html>
  <head:divvy>
    <title>Divvy!</title>
  </head:divvy>
  <body>
    <divvy:nav/>
    <div class="card">
      <div class="card-body">
        Divvy is a quick and fair way to split the cost of a trip with your friends.  To get started enter
        a trip name and the email addresses of those on the trip below.
        <hr/>
        <form method="post" action="#">
          <div id="form-inner-div" class="mb-3">
            <div class="input-group">
              <input type="text" name="title" class="form-control" placeholder="Trip name" required="true"/>
            </div>
            <div class="input-group">
              <input type="text" name="primary-name" class="form-control col-5" 
               placeholder="Your name" required="true"/>
              {
                $user ? 
                <input type="email" name="primary-email" value={$user->email} readonly={true}
                 class="form-control col-6 text-gray" />
                :
                <input type="email" name="primary-email" class="form-control col-6" 
                 placeholder="Your email address" required="true"/>
              }
            </div>
            <div class="input-group" id="other-email-div">
              <div class="input-group addon">
                <input type="text" name="name[]" class="form-control" placeholder="Other person name"
                 required="true" class="form-control col-5"/>
                <input type="email" name="email[]" class="form-control" placeholder="Other person email address"
                 required="true" class="form-control col-6"/>
                <button type="button" class="fas fa-plus input-group-addon btn-primary btn col-1"
                onclick="$('#other-email-div').clone()
                           .appendTo('#form-inner-div')
                           .find('input').val('')
                           .attr('placeholder','Another email address')
                           .removeAttr('required')"/>
              </div>
            </div>
          </div>
          <input type="submit" class="btn btn-primary w-100" value="Divvy!"/>
        </form>
      </div>
    </div>
    {
      $user && $events_list_xhp ?
      <div class="card">
        <div class="card-header h5">
          Events
        </div>
        <div class="card-body">
          {$events_list_xhp}
        </div>
      </div> : null
    }
  </body>
</html>;