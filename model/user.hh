<?hh
require_once('vendor/autoload.php');
require_once('vendor/hh_autoload.php');
require_once('model/model.hh');

use Josantonius\Session\Session;

Session::init(PHP_INT_MAX);

use Badpirate\HackTack\Model;
use Badpirate\HackTack\HT;

final class UserModel extends Model {
  public function __construct(
    public string $email
  ) {}

  static public function register(string $email) : string {
    $email = strtolower($email);
    $code = parent::generateRandomString(10);
    $stmt = parent::prepare(
      'INSERT INTO users (email, code)
       VALUES (?,?)
       ON DUPLICATE KEY UPDATE code = ?'
    );
    $stmt->bind_param('sss',&$email,&$code,&$code);
    parent::ec($stmt);
    return $code;
  }

  static public function existsForEmail(string $email) : bool {
    $email = strtolower($email);
    $stmt = parent::prepare(
      'SELECT email FROM users WHERE email = ?'
    );
    $stmt->bind_param('s',&$email);
    $result_email = null;
    $stmt->bind_result(&$result_email);
    parent::execute($stmt);
    if ($stmt->fetch()) {
      $stmt->close();
      return true;
    } else {
      $stmt->close();
      return false;
    }
  }

  static public function loginRedirect(string $email, ?string $url = null) {
    $email = strtolower($email);
    $current = $url ?? (isset($_SERVER['HTTPS']) && $_SERVER['HTTPS'] === 'on' ? "https" : "http") . "://$_SERVER[HTTP_HOST]$_SERVER[REQUEST_URI]";
    $current = urlencode($current);
    HT::redirect("login.hh?action=email&email=$email&r=$current");
  }

  public function setPassword(string $password) {
    $stmt = parent::prepare(
      'UPDATE users SET password = ? WHERE email = ?'
    );
    $hash = password_hash($password, PASSWORD_DEFAULT);
    $stmt->bind_param('ss', &$hash, &$this->email);
    parent::ec($stmt);
  }

  static public function logout(?string $redirect = null) {
    Session::pull('email');
    HT::redirect($redirect ?? 'index.hh');
  }

  public function login(?string $redirect = null) {
    Session::set('email', $this->email);
    HT::redirect($redirect ? $redirect : 'index.hh');
  }

  static public function forEmail(string $email, string $password) : ?UserModel {
    $email = strtolower($email);
    $stmt = parent::prepare(
      'SELECT password FROM users WHERE email = ?'
    );
    $stmt->bind_param('s',&$email);
    $hash = null;
    $stmt->bind_result(&$hash);
    parent::execute($stmt);
    if ($stmt->fetch()) {
      $stmt->close();
      if (password_verify($password, $hash)) {
        return new UserModel(
          $email
        );
      } else {
        return null;
      }
    } else {
      $stmt->close();
      return null;
    }
  }

  static public function forCode(string $code) : ?UserModel {
    $stmt = parent::prepare(
      'SELECT email FROM users WHERE code = ?'
    );
    $stmt->bind_param('s',&$code);
    $email = null;
    $stmt->bind_result(&$email);
    parent::execute($stmt);
    if ($stmt->fetch()) {
      $stmt->close();
      return new UserModel($email);
    } else {
      $stmt->close();
      return null;
    }
  }

  static public function fromSession() : ?UserModel {
    $email = Session::get('email');
    if (!$email) return null;
    return new UserModel(
      $email
    );
  }
}