<?hh
require_once('vendor/autoload.php');
require_once('vendor/hh_autoload.php');
require_once('template.hh');

require_once('model/user.hh');
require_once('model/email.hh');

$email = null;
$code_sent = false;
$c = null;
$error = null;
$redirect = isset($_REQUEST['r']) ? $_REQUEST['r'] : null;
if (isset($_REQUEST['c'])) {
  $c = $_REQUEST['c'];
  if($_SERVER['REQUEST_METHOD'] === 'POST') {
    $password = $_REQUEST['password'];
    $verify = $_REQUEST['verify'];
    if ($password !== $verify) {
      $error = "Passwords don't match";
    } else {
      $user = UserModel::forCode($c);
      if (!$user) {
        $error = "Invalid code.";
        $c = null;
      } else {
        $user->setPassword($password);
        $user->login($redirect);
      }
    }
  }
} else if (isset($_REQUEST['action'])) {
  switch($_REQUEST['action']) {
    case 'email':
      if (UserModel::existsForEmail($_REQUEST['email'])) {
        $email = strtolower($_REQUEST['email']);
      } else {
        $code = UserModel::register($_REQUEST['email']);
        MailModel::sendGeneric(
          $_REQUEST['email'],
          $_REQUEST['email'],
          'Register for Divvy',
          'Click below to register for Divvy and login!',
          'Register',
          getenv('DIVVY_SITE')."/login.hh?c=$code"
        );
        $code_sent = true;
      }
      break;
    case 'password':
      $user = UserModel::forEmail($_REQUEST['email'],$_REQUEST['password']);
      if (!$user) {
        $email = $_REQUEST['email'];
        $error = "Invalid login.";
      } else {
        $user->login($redirect);
      }
      break;
    default:
      throw new Exception("Unhandled action ".$_REQUEST['action']);
  }
}

print
<html>
  <head:divvy>
    <title>Divvy Login</title>
  </head:divvy>
  <body>
    <divvy:nav/>
    <div class="card">
      <div class="card-header h5">
        Login
      </div>
      <div class="card-body">
        {
          $error ?
          <div class="alert alert-danger h5">
            {$error}
          </div> : null
        }
        {
          $c ?
          <form method="post" class="input-group">
            <input type="password" placeholder="Password" name="password" class="form-control" required={true}/>
            <input type="password" placeholder="Verify" name="verify" class="form-control" required={true}/>
            { $redirect ? <input type="hidden" name="r" value={$redirect}/> : null }
            <input type="hidden" name="c" value={$c}/>
            <span class="input-group-append">
              <input type="submit" class="form-control" value="Set Password"/>
            </span>
          </form> 
          :(
            $code_sent ?
            <h5>Check your email (and spam) for a email from 'divvy@logichigh.com' to finish registration</h5> 
            :
            <form method="post">
              {
                $email ?
                <span class="input-group">
                  <input type="hidden" name="email" value={$email} class="form-control"/>
                  { $redirect ? <input type="hidden" name="r" value={$redirect}/> : null }
                  <input type="password" name="password" placeholder={"Password for $email"} required={true} 
                  class="form_control"/>
                  <input type="hidden" name="action" value="password"/>
                  <span class="input-group-append">
                    <input type="submit" class="form-control" value="Login"/>
                  </span>
                </span>
                :
                <span class="input-group">
                  <input type="email" placeholder="email" required={true} name="email" class="form-control"/>
                  { $redirect ? <input type="hidden" name="r" value={$redirect}/> : null }
                  <input type="hidden" name="action" value="email"/>
                  <span class="input-group-append">
                    <input type="submit" class="form-control" value="Login"/>
                  </span>
                </span>
              }
            </form>
          )
        }
      </div>
    </div>
  </body>
</html>;