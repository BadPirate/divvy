<?hh
require_once('vendor/autoload.php');
require_once('vendor/hh_autoload.php');

require_once('model/user.hh');
require_once('model/event.hh');
require_once('model/guest.hh');

final class :divvy:nav extends :x:element {
  attribute :nav;
  attribute ?EventModel event = null;

  use XHPHelpers;

  protected function render(): XHPRoot {
    $id = $this->getID();

    $user = UserModel::fromSession();
    $event = null;
    $cg = null;
    if ($this->:event) {
      $event = $this->:event;
      $cg = $this->:event->current;
      if (!$cg) throw new Exception("Current guest expected");
    }

    $user_select_menu_xhp = <div class="dropdown-menu"/>;
    if($event) {
      foreach($event->guests as $guest) {
        $user_select_menu_xhp->appendChild(
          <a href={"event.hh?id=$event->id&g=$guest->id"} class="dropdown-item">
            {$guest->name}
          </a>
        );
      }
    }

    $user_select_xhp =
    $user ? 
    <div class="nav-item active">
      <a class="nav-link ml-auto" href="logout.hh">
        Logout ({$user->email})
      </a>
    </div>
    :(
      $event && $cg ?
      <div class="nav-item dropdown">
        <a class="nav-link dropdown-toggle" data-toggle="dropdown" href="#">{$cg->name}</a>
        {$user_select_menu_xhp}
      </div>
      :
      <div class="nav-item active">
        <a class="nav-link ml-auto" href="login.hh">
          Login
        </a>
      </div>
    );

    $nav = 
      <nav class="navbar navbar-light bg-light">
        <a class="navbar-brand" href="index.hh">
          <img src="img/icon.gif"/>
          <span class="dvhead">
            Divvy
          </span>
        </a>
        {
          $event ?
          <div class="nav-item active mr-auto ml-auto font-weight-bold">
            {$event->title}
          </div> : null
        }
        {$user_select_xhp}
      </nav>;

    $this->transferAllAttributes($nav);
  
    return $nav;
  }
}

final class :head:divvy extends :head:jstrap {
  protected function render(): XHPRoot {
    $head = parent::render();
    $head->appendChild(
      <link rel="stylesheet" href="template.css" type="text/css"/>
    );
    return $head;
  }
}