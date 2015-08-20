<?php
require_once("Exception.php");
require_once("openmediavault/util.inc");
require_once("Image.php");
require_once("Container.php");


/**
 * Helper class for Docker module
 */
class OMVModuleDockerUtil {

	/**
	 * Returns an array with Image objects on the system
	 *
	 * @return array $objects An array with Image objects
	 *
	 */
	public static function getImages($apiPort, $incDangling) {
		$objects=array();
		$curl = curl_init();
		curl_setopt_array($curl, array(
			CURLOPT_RETURNTRANSFER => 1,
			CURLOPT_TIMEOUT => 30,
			CURLOPT_CONNECTTIMEOUT => 5
		));
		$url = "http://localhost:" . $apiPort . "/images/json?all=0";
		/*
		if($incDangling) {
			$url .= "0";
		} else {
			$url .= "1";
		}
		 */
		curl_setopt($curl, CURLOPT_URL, $url);
		if(!($response = curl_exec($curl))){
			throw new OMVModuleDockerException('Error: "' . curl_error($curl) . '" - Code: ' . curl_errno($curl));
		}
		curl_close($curl);
		$data = array();
		foreach(json_decode($response) as $item) {
			$data[substr($item->Id, 0, 12)] = $item;
		}
		foreach($data as $item) {	
			$image = new OMVModuleDockerImage($item->Id, $data, $apiPort);
			$tmp=array(
				"repository"=>rtrim(ltrim($image->getRepository(), "<"), ">"),
				"tag"=>rtrim(ltrim($image->getTag(), "<"), ">"),
				"id"=>$image->getId(),
				"created"=>$image->getCreated(),
				"size"=>$image->getSize(),
				"ports"=>$image->getPorts(),
				"envvars"=>$image->getEnvVars());
			array_push($objects, $tmp);
		}
		return $objects;
	}
	
	/**
	 * Returns a single image from it's ID
	 *
	 * @return OMVModuleDockerImage $image A single Docker image
	 *
	 */
	public static function getImage($id, $apiPort) {
		$objects = array();
		$curl = curl_init();
		curl_setopt_array($curl, array(
			CURLOPT_RETURNTRANSFER => 1,
			CURLOPT_TIMEOUT => 30,
			CURLOPT_CONNECTTIMEOUT => 5
		));
		$url = "http://localhost:" . $apiPort . "/images/json?all=1";
		curl_setopt($curl, CURLOPT_URL, $url);
		if(!($response = curl_exec($curl))){
			throw new OMVModuleDockerException('Error: "' . curl_error($curl) . '" - Code: ' . curl_errno($curl));
		}
		curl_close($curl);
		$data = array();
		foreach(json_decode($response) as $item) {
			$data[substr($item->Id, 0, 12)] = $item;
		}
		return (new OMVModuleDockerImage($data[$id]->Id, $data, $apiPort));
	}


	/**
	 * Returns an array with Container objects on the system
	 *
	 * @return array $objects An array with Container objects
	 *
	 */
	public static function getContainers($apiPort) {
		$objects = array();
		$curl = curl_init();
		curl_setopt_array($curl, array(
			CURLOPT_RETURNTRANSFER => 1,
			CURLOPT_TIMEOUT => 30,
			CURLOPT_CONNECTTIMEOUT => 5
		));
		$url = "http://localhost:" . $apiPort . "/containers/json?all=1";
		curl_setopt($curl, CURLOPT_URL, $url);
		if(!($response = curl_exec($curl))){
			throw new OMVModuleDockerException('Error: "' . curl_error($curl) . '" - Code: ' . curl_errno($curl));
		}
		curl_close($curl);
		$data = array();
		foreach(json_decode($response) as $item) {
			$data[substr($item->Id, 0, 12)] = $item;
		}
		foreach($data as $item) {	
			$container = new OMVModuleDockerContainer($item->Id, $data, $apiPort);
			$ports = "";
			foreach($container->getPorts() as $exposedport => $hostports) {
				if($hostports) {
					foreach($hostports as $hostport) {
						$ports .= $hostport["HostIp"] . ":" . $hostport["HostPort"] . "->" . $exposedport . ", ";
					}
				} else {
					$ports .= $exposedport . ", ";
				}
			}
			$image = OMVModuleDockerUtil::getImage(substr($container->getImageId(), 0, 12), $apiPort);
			$exposedPorts = $image->getPorts();
			$envvars = $image->getEnvVars();
			$ports = rtrim($ports, ", ");
			$obj = array(
				"id" => $container->getId(),
				"image" => $container->getImage(),
				"command" => $container->getCommand(),
				"created" => $container->getCreated(),
				"state" => $container->getState(),
				"status" => $container->getStatus(),
				"name" => $container->getName(),
				"privileged" => $container->getPrivileged(),
				"restartpolicy" => $container->getRestartPolicy(),
				"networkmode" => ucfirst($container->getNetworkMode()),
				"envvars" => $envvars,
				"cenvvars" => $container->getEnvironmentVariables(),
				"exposedports" => $exposedPorts,
				"portbindings" => $container->getPortBindings(),
				"bindmounts" => $container->getBindMounts(),
				"ports" => $ports);
			array_push($objects, $obj);
		}
		return $objects;
	}

	/**
	 * Returns a single container from it's ID
	 *
	 * @return OMVModuleDockerContainer $container A single container object
	 *
	 */
	public static function getContainer($id, $apiPort) {
		$objects = array();
		$curl = curl_init();
		curl_setopt_array($curl, array(
			CURLOPT_RETURNTRANSFER => 1,
			CURLOPT_TIMEOUT => 30,
			CURLOPT_CONNECTTIMEOUT => 5
		));
		$url = "http://localhost:" . $apiPort . "/containers/json?all=1";
		curl_setopt($curl, CURLOPT_URL, $url);
		if(!($response = curl_exec($curl))){
			throw new OMVModuleDockerException('Error: "' . curl_error($curl) . '" - Code: ' . curl_errno($curl));
		}
		curl_close($curl);
		$data = array();
		foreach(json_decode($response) as $item) {
			$data[substr($item->Id, 0, 12)] = $item;
		}
		return (new OMVModuleDockerContainer($data[$id]->Id, $data, $apiPort));
	}

	/**
	 * Returns a string representing a time sometime in the past
	 *
	 * @return string $when A string representaion of a past time
	 *
	 */
	public static function getWhen($now, $eventTime) {
		$when = "";
		$diff = date_diff(new DateTime($now),new DateTime($eventTime));
		if($diff->y > 0) {
			$when = "$diff->y years";
		} elseif($diff->m > 0) {
			$when = "$diff->m months";
		} elseif($diff->d > 0) {
			$when = "$diff->d days";
		} elseif($diff->h > 0) {
			$when = "$diff->h hours";
		} elseif($diff->i > 0) {
			$when = "$diff->i minutes";
		} elseif($diff->s > 0) {
			$when = "$diff->s seconds";
		} else {
			$when = "Less than a second";
		}
		return $when;
	}

	/**
	 * Convert bytes to human readable format
	 *
	 * @param integer bytes Size in bytes to convert
	 * @return string
	 */
	function bytesToSize($bytes, $precision =1)
	{ 
		/* 
		$kilobyte = 1024;
		$megabyte = $kilobyte * 1024;
		$gigabyte = $megabyte * 1024;
		$terabyte = $gigabyte * 1024;
		 */

		$kilobyte = 1000;
		$megabyte = $kilobyte * 1000;
		$gigabyte = $megabyte * 1000;
		$terabyte = $gigabyte * 1000;

		if (($bytes >= 0) && ($bytes < $kilobyte)) {
			return $bytes . ' B';

		} elseif (($bytes >= $kilobyte) && ($bytes < $megabyte)) {
			return round($bytes / $kilobyte, $precision) . ' KB';

		} elseif (($bytes >= $megabyte) && ($bytes < $gigabyte)) {
			return round($bytes / $megabyte, $precision) . ' MB';

		} elseif (($bytes >= $gigabyte) && ($bytes < $terabyte)) {
			return round($bytes / $gigabyte, $precision) . ' GB';

		} elseif ($bytes >= $terabyte) {
			return round($bytes / $terabyte, $precision) . ' TB';
		} else {
			return $bytes . ' B';
		}
	}

	/**
	 * Change the Docker daemon settings
	 *
	 * @param string apiPort The new API port to use
	 */
	function changeDockerSettings($apiPort)
	{ 
		//Stop the Docker daemon before making config changes
		$cmd = "service docker stop";
		OMVUtil::exec($cmd, $out, $res);
		
		$fileName = "/etc/default/docker";
		$data = file_get_contents($fileName);
		$lines = explode("\n", $data);
		$result = "";
		$socketSet = false;
		foreach($lines as $line) {
			if(strcmp($line, "### Do not change these lines. They are added and updated by the OMV Docker GUI plugin.") === 0) {
				break;
			} else {
				if(preg_match('/^DOCKER_OPTS.*unix\:\/\/\/var\/run\/docker\.sock.*$/', $line)) {
					$socketSet = true;
				}
				$result .= $line . "\n";
			}
		}
		$result .= '### Do not change these lines. They are added and updated by the OMV Docker GUI plugin.' . "\n";
		if($socketSet) {
			$result .= 'OMVDOCKER_API="-H tcp://127.0.0.1:' . $apiPort . '"' . "\n";
		} else {
			$result .= 'OMVDOCKER_API="-H unix:///var/run/docker.sock -H tcp://127.0.0.1:' . $apiPort . '"' . "\n";

		}

		$result .= 'OMVDOCKER_IMAGE_PATH=""' . "\n" .
			'DOCKER_OPTS="$DOCKER_OPTS $OMVDOCKER_API $OMVDOCKER_IMAGE_PATH"' . "\n" .
			'### Do not add any configuration below this line. They will be removed when the plugin is removed';

		file_put_contents("$fileName", $result);

		//Start the daemon again after changes have been made
		$cmd = "service docker start";
		OMVUtil::exec($cmd, $out, $res);
	}

	/**
	 * Helper function to execute a command and throw an exception on error
	 * (requires stderr redirected to stdout for proper exception message).
	 * 
	 * @param string $cmd Command to execute
	 * @param array &$out If provided will contain output in an array
	 * @param int &$res If provided will contain Exit status of the command
	 * @return string Last line of output when executing the command
	 * @throws OMVModuleDockerException
	 * @access public
	 */
	public static function exec($cmd, &$out = null, &$res = null) {
		$tmp = OMVUtil::exec($cmd, $out, $res);
		if ($res) {
			throw new OMVModuleDockerException(implode("\n", $out));
		}
		return $tmp;
	}

}

?>
