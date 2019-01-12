<?hh
require_once('vendor/autoload.php');
require_once('vendor/hh_autoload.php');

$dotenv = Dotenv\Dotenv::create(__DIR__, 'config.env');
$dotenv->load();