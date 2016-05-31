<?php
/**
 * Copyright (c) 2015-2016 OpenMediaVault Plugin Developers
 *
 * @category OMVRpcServiceDocker
 * @package  Openmediavault-docker-gui
 * @author   OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @license  http://www.gnu.org/copyleft/gpl.html GNU General Public License
 * @link     https://github.com/OpenMediaVault-Plugin-Developers/openmediavault-docker-gui
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

require_once "openmediavault/globals.inc";
require_once "openmediavault/functions.inc";
require_once "/usr/share/omvdocker/Utils.php";
require_once "/usr/share/omvdocker/Image.php";

/**
 * RPC class for the Docker GUI plugin
 *
 * @category Class
 * @package  Openmediavault-docker-gui
 * @author   OpenMediaVault Plugin Developers <plugins@omv-extras.org>
 * @license  http://www.gnu.org/copyleft/gpl.html GNU General Public License
 * @link     https://github.com/OpenMediaVault-Plugin-Developers/openmediavault-docker-gui
 *
 */
class OMVRpcServiceDocker extends OMVRpcServiceAbstract
{
    /**
     * Get the name of the RPC service
     *
     * @return string
     */
    public function getName()
    {
        return "Docker";  // RPC Service name. Same as in .js files
    }

    /**
     * Get the main event message path of the service. This is a helper
     * function to avoid "magic numbers".
     *
     * @return string
     */
    private function _getEventMessagePath()
    {
        return "org.openmediavault.services.docker";
    }

    /**
     * Get the base XPath of the service. This is a helper function to avoid
     * "magic numbers".
     *
     * @return string
     */
    private function _getXpath()
    {
        return "/config/services/docker";
    }

    /**
     * Initialize the RPC service. Different methods of the RPC service are
     * declared here
     *
     * @return void
     */
    public function initialize()
    {
        $this->registerMethod("getImages");
        $this->registerMethod("pullImage");
        $this->registerMethod("deleteImage");
        $this->registerMethod("searchImages");
        $this->registerMethod("getImageData");

        $this->registerMethod("getContainers");
        $this->registerMethod("runContainer");
        $this->registerMethod("modifyContainer");
        $this->registerMethod("startContainer");
        $this->registerMethod("stopContainer");
        $this->registerMethod("restartContainer");
        $this->registerMethod("deleteContainer");
        $this->registerMethod("createContainer");
        $this->registerMethod("getVolumesFrom");
        $this->registerMethod("commitContainer");
        $this->registerMethod("modifyWarningEnabled");
        $this->registerMethod("refreshContainers");
        $this->registerMethod("getContainerData");

        $this->registerMethod("getSettings");
        $this->registerMethod("setSettings");
        $this->registerMethod("getDetails");
        $this->registerMethod("getLogs");
        $this->registerMethod("getRootFolders");
        $this->registerMethod("copyDockerData");
        $this->registerMethod("executeCommand");
        $this->registerMethod("syncDockerLogos");
        $this->registerMethod("getDockerRepo");
    }

    /**
     * Get all Docker images
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $objects An array with all Docker images
     */
    public function getImages($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $settings = $db->get('conf.service.docker');

        $objects = [];
        if ($settings->get('enabled')) {
            if ($settings->get('showDanglingImages')) {
                $incDangling = false;
            } else {
                $incDangling = true;
            }

            $objects = OMVModuleDockerUtil::getImageList(
                $settings->get('apiPort'),
                $incDangling
            );
        }
        return $objects;
    }

    /**
     * Pull a Docker image from the Docker Hub
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return void
     */
    public function pullImage($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.docker.pullimage");

        if (strcmp($params['tag'], "") === 0) {
            $params['tag'] = "latest";
        }

        // Create a background process.
        $bgStatusFilename = $this->createBgProcStatus();
        $pid = $this->fork();
        if ($pid > 0) {
            $this->initializeBgProcStatus($bgStatusFilename, $pid);
            return $bgStatusFilename;
        }

        // Child process.
        try {
            $bgOutputFilename = $this->createBgProcOutput();
            $this->updateBgProcStatus(
                $bgStatusFilename,
                "outputfilename",
                $bgOutputFilename
            );
            $cmd = sprintf("docker pull \"%s:%s\"",
                $params['repository'],
                $params['tag']
            );
            $this->exec($cmd, $output, $bgOutputFilename);
            $this->finalizeBgProcStatus($bgStatusFilename, $output);

        } catch(\Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }

    /**
     * Deletes a Docker image
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return void
     */
    public function deleteImage($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.docker.deleteimage");

        $cmd = sprintf("docker rmi %s", $params['id']);
        OMVModuleDockerUtil::exec($cmd, $out, $res);
    }

    /**
     * Searches the Docker Hub for a specific repository
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $objects An array ith all repositories found
     */
    public function searchImages($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $settings = $db->get("conf.service.docker");

        $objects = [];
        $url = sprintf("http://localhost:%s/images/search?term=%s",
            $settings->get('apiPort'), $params["name"]
        );
        $response = OMVModuleDockerUtil::doApiCall($url);
        $data = json_decode($response);

        foreach ($data as $repo) {
            switch ($params['filterParam']) {
            case "all":
                array_push(
                    $objects,
                    [
                        "name" => $repo->name,
                        "description" => $repo->description,
                        "stars" => $repo->star_count
                    ]
                );
                break;
            case "trusted":
                if ($repo->is_trusted) {
                    array_push(
                        $objects,
                        [
                            "name" => $repo->name,
                            "description" => $repo->description,
                            "stars" => $repo->star_count
                        ]
                    );
                }
                break;
            case "official":
                if ($repo->is_official) {
                    array_push(
                        $objects,
                        [
                            "name" => $repo->name,
                            "description" => $repo->description,
                            "stars" => $repo->star_count
                        ]
                    );
                }
                break;
            case "trustedofficial":
                if ($repo->is_official && $repo->is_trusted) {
                    array_push(
                        $objects,
                        [
                            "name" => $repo->name,
                            "description" => $repo->description,
                            "stars" => $repo->star_count
                        ]
                    );
                }
                break;
            default:
                array_push(
                    $objects,
                    [
                        "name" => $repo->name,
                        "description" => $repo->description,
                        "stars" => $repo->star_count
                    ]
                );
                break;
            }
        }
        return $objects;
    }

    /**
     * Get data of specific Image
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $objects An array ith all repositories found
     */
    public function getImageData($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $settings = $db->get("conf.service.docker");

        $image = OMVModuleDockerUtil::getImage($params['id'], $settings->get('apiPort'));

        $object = [
            "repository" => rtrim(ltrim($image->getRepository(), "<"), ">"),
            "tag" => rtrim(ltrim($image->getTag(), "<"), ">"),
            "id" => $image->getId(),
            "created" => $image->getCreated(),
            "size" => $image->getSize(),
            "ports" => $image->getPorts(),
            "envvars" => $image->getEnvVars(),
            "imagevolumes" => $image->getVolumes()
        ];
        return $object;
    }

    /**
     * Get all Docker containers
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $objects An array with all containers
     */
    public function getContainers($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $settings = $db->get("conf.service.docker");

        $objects = [];

        if ($settings->get('enabled')) {
            $objects = OMVModuleDockerUtil::getContainerList(
                $settings->get('apiPort')
            );
        }
        return $objects;
    }

    /**
     * Run a single Docker container
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return void
     */
    public function runContainer($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        $cmd = "docker run -d ";

        //Check if restart checkbox is enabled
        if ($params['restart']) {
            $cmd .= "--restart=always ";
        }

        //Check if privileged checkbox is enabled
        if ($params['privileged']) {
            $cmd .= "--privileged ";
        }

        //Check if time sync checkbox is enabled
        if ($params['timeSync']) {
            $cmd .= "-v /etc/localtime:/etc/localtime:ro ";
        }

        //Check which networking mode is selected
        switch ($params['networkMode']) {
        case "Bridge":
            $cmd .= "--net=bridge ";
            foreach ($params['portForwards'] as $portForward) {
                if (!(strcmp($portForward['customPort'], "") === 0)) {
                    $destport = $portForward['customPort'] . "/" . $portForward['proto'];
                } else {
                    $destport = $portForward['exposedPort'];
                }
                if (!(strcmp($portForward['hostip'], "") === 0)) {
                    $cmd .= "-p " . $portForward['hostip'] . ":" .
                        $portForward['hostport'] . ":" . $destport . " ";
                }
            }
            //Check if host name is specified
            if (!(strcmp($params['hostName'], "") === 0)) {
                $cmd .= "-h \"" . $params['hostName'] . "\" ";
            }
            break;
        case "Host":
            $cmd .= "--net=host ";
            break;
        case "None":
            $cmd .= "--net=none ";
            break;
        }

        //Check for environment variables
        foreach ($params['envVars'] as $envVar) {
            if (!(strcmp($envVar['name'], "") === 0)) {
                $cmd .= "-e " . $envVar['name'] . "=\"" . $envVar['value'] . "\" ";
            }
        }

        //Check for bind mounts
        //Container path must not be set to "/"
        //If Container path is empty a new volume is created instead
        //of bindmount
        foreach ($params['bindMounts'] as $bindMount) {
            if (strcmp($bindMount['to'], "/") === 0) {
                throw new OMVModuleDockerException(
                    "Container path must not be set to \"/\""
                );
            }
            if (!(strcmp($bindMount['from'], "") === 0)) {
                if (!(strcmp($bindMount['to'], "") === 0)) {
                    $cmd .= "-v \"" . $bindMount['from'] . "\":\"" .
                        $bindMount['to'] . "\"";
                    if ($bindMount['romode']) {
                        $cmd .= ":ro ";
                    } else {
                        $cmd .= ":rw ";
                    }
                } else {
                    $cmd .= "-v \"" . $bindMount['from'] . "\" ";
                }
            }
        }

        //Check for volumes from
        foreach ($params['volumes'] as $volume) {
            if (!(strcmp($volume['from'], "") === 0)) {
                $cmd .= "--volumes-from \"" . $volume['from'] . "\" ";
            }
        }

        //Check if a custom container name has been supplied
        if (!(strcmp($params['containerName'], "") === 0)) {
            $cmd .= "--name=\"" . $params['containerName'] . "\" ";
        }

        //Check if extra arguments are supplied
        $extraArgs = trim($params['extraArgs']);
        if (!(strcmp($extraArgs, "") === 0)) {
            $cmd .= $extraArgs . " ";
        }

        //Add image to command
        $cmd .= "\"" . $params['image'] . "\" 2>&1";

        //throw new OMVModuleDockerException($cmd);

        OMVModuleDockerUtil::exec($cmd, $out, $res);

    }

    /**
     * Modify a single Docker container
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return void
     */
    public function modifyContainer($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $settings = $db->get("conf.service.docker");

        $container = OMVModuleDockerUtil::getContainer($params['cid'], $settings->get('apiPort'));
        $image = OMVModuleDockerUtil::getImage(
            substr($container->getImageId(), 0, 12),
            $apiPort
        );

        $restart = false;
        if (strcmp($container->getRestartPolicy(), "always") === 0) {
            $restart = true;
        }
        $portForwards = [];
        foreach ($container->getPortBindings() as $binding) {
            $customPort = "";
            $exposedPort = "";
            foreach ($image->getPorts() as $port) {
                if (strcmp($binding['containerportstring'], $port['name']) === 0) {
                    $exposedPort = explode("/", $port['name'])[0];
                    break;
                }
            }
            if (strcmp($exposedPort, "") === 0) {
                $customPort = explode("/", $binding['containerportstring'])[0];
            }
            array_push($portForwards,
                [
                    "customPort" => $customPort,
                    "exposedPort" => $exposedPort,
                    "hostip" => $binding['hostip'],
                    "hostport" => $binding['hostport'],
                    "proto" => $binding['proto']
                ]
            );

        }
        $envVars = [];
        foreach ($container->getEnvironmentVariables() as $key => $value) {
            array_push($envVars,
                [
                    "name" => $key,
                    "value" => $value
                ]
            );
        }
        $bindMounts = [];
        foreach($container->getBindMounts() as $mount) {
            array_push($bindMounts,
                [
                    "from" => $mount['from'],
                    "to" => $mount['to'],
                    "romode" => $mount['mode']
                ]
            );
        }

        $old_params = [
            "restart" => $restart,
            "privileged" => $container->getPrivileged(),
            "timeSync" => $container->syncsTime(),
            "networkMode" => ucfirst($container->getNetworkMode()),
            "portForwards" => $portForwards,
            "hostName" => $container->getHostName(),
            "envVars" => $envVars,
            "bindMounts" => $bindMounts,
            "volumes" => $container->getVolumesFrom(),
            "containerName" => $container->getName(),
            "image" => $container->getImage()
        ];

        //Remove existing container
        $cmd = "docker stop " . $params['cid'] . " && docker rm " . $params['cid'];
        OMVModuleDockerUtil::exec($cmd, $out, $res);

        if ($params['refresh']) {
            $params = $old_params;
        }

        //Try to start up a new container with the given parameters.
        //Revert to old parameters upon failure
        try {
            $this->runContainer($params, $context);
        } catch (OMVModuleDockerException $e) {
            $this->runContainer($old_params, $context);
            $e_str = explode("'", (string)$e);
            return (["response" => $e_str[3]]);
        }
    }

    /**
     * Start one or multiple Docker containers
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return void
     */
    public function startContainer($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        $cmd = "docker start " . $params['id'] . " 2>&1";
        OMVModuleDockerUtil::exec($cmd, $out, $res);
    }

    /**
     * Stop one or multiple Docker containers
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return void
     */
    public function stopContainer($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        $cmd = "docker stop " . $params['id'] . " 2>&1";
        OMVModuleDockerUtil::exec($cmd, $out, $res);
    }

    /**
     * Restart one or multiple Docker containers
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return void
     */
    public function restartContainer($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        $cmd = "docker restart " . $params['id'] . " 2>&1";
        OMVModuleDockerUtil::exec($cmd, $out, $res);
    }

    /**
     * Delete one or multiple Docker containers
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return void
     */
    public function deleteContainer($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.docker.deleteimage");

        $cmd = "docker rm " . $params['id'] . " 2>&1";
        OMVModuleDockerUtil::exec($cmd, $out, $res);
    }

    /**
     * Create a new Docker container
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return void
     */
    public function createContainer($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        $cmd = "docker create ";

        //Check for bind mounts
        //Container path must not be set to "/"
        //If Container path is empty a new volume is created instead
        //of bindmount
        foreach ($params['bindMounts'] as $bindMount) {
            if (strcmp($bindMount['to'], "/") === 0) {
                throw new OMVModuleDockerException(
                    "Container path must not be set to \"/\""
                );
            }
            if (!(strcmp($bindMount['from'], "") === 0)) {
                if (!(strcmp($bindMount['to'], "") === 0)) {
                    $cmd .= "-v \"" . $bindMount['from'] . "\":\"" .
                        $bindMount['to'] . "\" ";
                } else {
                    $cmd .= "-v \"" . $bindMount['from'] . "\" ";
                }
            }
        }

        //Check if a custom container name has been supplied
        if (!(strcmp($params['containerName'], "") === 0)) {
            $cmd .= "--name=" . $params['containerName'] . " ";
        }

        $cmd .= "\"" . $params['imageRepo'] . "\" /bin/true";
        OMVModuleDockerUtil::exec($cmd, $out, $res);
    }

    /**
     * Get all containers with volumes mounted
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $objects An array with all containers
     */
    public function getVolumesFrom($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $settings = $db->get('conf.service.docker');

        $objects = [];

        if ($settings->get('enabled')) {
            $containers = OMVModuleDockerUtil::getContainers(
                $settings->get('apiPort')
            );
            foreach ($containers as $container) {
                if ($container['hasmounts']) {
                    array_push($objects, ["name" => $container['name']]);
                }
            }
        }
        return $objects;
    }

    /**
     * Commit a container to a new image
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return void
     */
    public function commitContainer($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        $cmd = "docker commit " . $params['name'] . " " . $params['newImage'] . " 2>&1";
        OMVModuleDockerUtil::exec($cmd, $out, $res);
    }

    /**
     * Check if a warning should be displayed when modifying a container
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return void
     */
    public function modifyWarningEnabled($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $settings = $db->get('conf.service.docker');

        if ($settings-get('cwarn')) {
            return ["response" => "true"];
        } else {
            return null;
        }
    }

    /**
     * Refresh all containers based on a specific image
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return void
     */
    public function refreshContainers($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $settings = $db->get('conf.service.docker');

        $image = $params['image'];
        $containers = OMVModuleDockerUtil::getContainers($settings->get('apiPort'));

        foreach ($containers as $c) {
            if (strcmp($c['image'], $image) === 0) {
                $new_params = [
                    "cid" => $c['id'],
                    "refresh" => true
                ];
                $this->modifyContainer($new_params, $context);
            }
        }
    }

    /**
     * Get data of a single container
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return void
     */
    public function getContainerData($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $settings = $db->get('conf.service.docker');

        $container = OMVModuleDockerUtil::getContainer($params['id'], $settings->get('apiPort'));
        $ports = "";
        foreach ($container->getPorts() as $exposedport => $hostports) {
            if ($hostports) {
                foreach ($hostports as $hostport) {
                    $ports .= $hostport["HostIp"] . ":" .
                        $hostport["HostPort"] .
                        "->" . $exposedport . ", ";
                }
            } else {
                $ports .= $exposedport . ", ";
            }
        }
        $ports = rtrim($ports, ", ");
        $image = OMVModuleDockerUtil::getImage(
            substr($container->getImageId(), 0, 12),
            $settings->get('apiPort')
        );

        $object = [
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
            "envvars" => $image->getEnvVars(),
            "cenvvars" => $container->getEnvironmentVariables(),
            "exposedports" => $image->getPorts(),
            "portbindings" => $container->getPortBindings(),
            "bindmounts" => $container->getBindMounts(),
            "ports" => $ports,
            "hasmounts" => $container->hasMounts(),
            "volumesfrom" => $container->getVolumesFrom(),
            "hostname" => $container->getHostName(),
            "timesync" => $container->syncsTime(),
            "imagevolumes" => $image->getVolumes()
        ];
        return $object;
    }

    /**
     * Retrieve all plugin settings
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $object An ssociative array with all settings
     */
    public function getSettings($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $settings = $db->get('conf.service.docker');

        $dockerVersion = "0";
        $dockerVersionInfo = "";
        $cmd = "docker --version 2>&1";
        OMVUtil::exec($cmd, $out, $res);

        if (!preg_match('/^Docker version ([\d\.]+)\,.*$/', $out[0], $matches)) {
            $dockerVersionInfo = "Docker not installed";
        } else {
            $dockerVersion = $matches[1];
            $cmd = "docker version";
            unset($out);
            OMVUtil::exec($cmd, $out, $res);
            $dockerVersionInfo = implode("\n", $out);
        }
        $object->set('version', $dockerVersion);
        $object->set('versionInfo', $dockerVersionInfo);

        return $object;
    }

    /**
     * Set all plugin settings
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $object An associative array with all the new settings
     */
    public function setSettings($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.docker.setsettings");

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $settings = $db->get('conf.service.docker');

        //Create the folder used for bind mounting when relocating
        //the base path
        $cmd = "mkdir -p /var/lib/docker/openmediavault";
        OMVModuleDockerUtil::exec($cmd, $out, $res);
        unset($out);

        $newApiPort = $settings->get('apiPort');
        $sharedFolder = $settings->get('sharedfolderref');

        //Make sure that the plugin is enabled to avoid unnecessary restarts of
        //the docker service.
        if ($params['enabled']) {
            $newApiPort = $params['apiPort'];
            $absPath = "";
            $sharedFolder = "";
            if (!(strcmp($params['sharedfolderref'], "") === 0)) {
                $absPath = OMVRpc::exec(
                    "ShareMgmt",
                    "getPath",
                    [
                        "uuid" => $params['sharedfolderref']
                    ],
                    $context
                );
                $sharedFolder = $params['sharedfolderref'];
            }
            OMVModuleDockerUtil::changeDockerSettings($newApiPort, $absPath);
        }

        //Get the possibly changed settings object
        $settings = $db->get('conf.service.docker');

        if (isset($settings->get('dockermntent'))) {
            $tmpMntent = $settings->get('dockermntent');
        } else {
            $tmpMntent = "";
        }

        // Update the configuration object.
        $settings->setAssoc($params);
        $settings->set('dockermntent', $tmpMntent);
        $settings->set('apiPort', $newApiPort);
        $settings->set('sharedfolderref', $sharedFolder);
        $db->set($settings);

        // Notify configuration changes.
        $dispatcher = \OMV\Engine\Notify\Dispatcher::getInstance();
        $dispatcher->notify(
            OMV_NOTIFY_MODIFY,
            "org.openmediavault.system.resetperms",
            $settings->getAssoc()
        );
        // Return the configuration object.
        return $settings->getAssoc();
    }

    /**
     * Get detailed info on a Docker image or container
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $object An associative array with all the new settings
     */
    public function getDetails($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        $cmd = sprintf("docker inspect %s 2>&1", $params['id']);
        OMVModuleDockerUtil::exec($cmd, $out, $res);

        return ["details" => implode("\n\r", $out)];
    }

    /**
     * Get logs from a Docker container
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $object An associative array with all the new settings
     */
    public function getLogs($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        $cmd = sprintf("docker logs %s 2>&1", $params['id']);
        OMVModuleDockerUtil::exec($cmd, $out, $res);
        return ["logs" => implode("\n\r", $out)];
    }

    /**
     * Get folder structure for folder browser
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $result An array with folders
     */
    public function getRootFolders($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // List the directory contents.
        $result = [];
        $path = sprintf("/%s", rtrim($params['path'], "/"));
        if (strlen($path) == 0) {
            $path = "/";
        }
        foreach (new DirectoryIterator($path) as $item) {
            if ($item->isDot()) {
                continue;
            }
            if (!$item->isDir()) {
                continue;
            }
            $result[] = $item->getFilename();
        }
        return $result;
    }

    /**
     * Copy Docker data from one folder to another
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return void
     */
    public function copyDockerData($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.docker.copydockerdata");

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();

        $absPath = "";
        if ($params['sharedfolderref']) {
            $xpath = "//system/shares/sharedfolder[uuid='" .
                $params['sharedfolderref'] . "']";
            $share = $xmlConfig->get($xpath);
            $xpath = "//system/fstab/mntent[uuid='" . $share['mntentref'] . "']";
            $mntent = $xmlConfig->get($xpath);
            $absPath = rtrim(
                rtrim(
                    $mntent['dir'], "/"
                )
                . "/" . $share['reldirpath'], "/"
            );
        }
        if ((strcmp($params['srcpath'], "") === 0) || (strcmp($absPath, "") === 0)) {
            throw new OMVModuleDockerException(
                "Either source or destination " .
                "path is empty"
            );
        }

        $srcpath = $params['srcpath'];
        $destpath = $absPath;
        OMVModuleDockerUtil::stopDockerService();

        if (file_exists($srcpath . "/containers") && file_exists($srcpath . "/volumes")) {
            if (file_exists($srcpath . "/devicemapper/mnt")) {
                $cmd = "umount $srcpath/devicemapper/mnt/*";
                OMVUtil::exec($cmd, $out, $res);
            }
            // Create a background process.
            $bgStatusFilename = $this->createBgProcStatus();
            $pid = $this->fork();
            if ($pid > 0) {
                $this->initializeBgProcStatus($bgStatusFilename, $pid);
                return $bgStatusFilename;
            }

            // Child process.
            try {
                $bgOutputFilename = $this->createBgProcOutput();
                $this->updateBgProcStatus(
                    $bgStatusFilename,
                    "outputfilename",
                    $bgOutputFilename
                );
                $cmd = sprintf("cp -R %s/* %s", $srcpath, $destpath);
                $this->exec($cmd, $output, $bgOutputFilename);
                $this->finalizeBgProcStatus($bgStatusFilename, $output);

            } catch(\Exception $e) {
                $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
                exit(1);
            }
        } else {
            OMVModuleDockerUtil::startDockerService();
            throw new OMVModuleDockerException(
                "Not a valid Docker source path specified"
            );
        }
        OMVModuleDockerUtil::startDockerService();
    }

    /**
     * Execute command inside running Docker container
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return void
     */
    public function executeCommand($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.docker.executecommand");

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();

        // Create a background process.
        $bgStatusFilename = $this->createBgProcStatus();
        $pid = $this->fork();
        if ($pid > 0) {
            $this->initializeBgProcStatus($bgStatusFilename, $pid);
            return $bgStatusFilename;
        }

        // Child process.
        try {
            $bgOutputFilename = $this->createBgProcOutput();
            $this->updateBgProcStatus(
                $bgStatusFilename,
                "outputfilename",
                $bgOutputFilename
            );
            $cmd = sprintf("docker exec -t %s %s",
                $params['containerId'], $params['cmd']
            );
            $this->exec($cmd, $output, $bgOutputFilename);
            $this->finalizeBgProcStatus($bgStatusFilename, $output);
        } catch(\Exception $e) {
            $this->finalizeBgProcStatus($bgStatusFilename, "", $e);
            exit(1);
        }
    }

    /**
     * Retrieve Docker logos from various sources
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $objects An associative array with all repos
     */
    public function syncDockerLogos($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        $logoPath = "/var/www/openmediavault/images/dockerrepo/";
        $cmd = "mkdir -p $logoPath";
        OMVUtil::exec($cmd, $out, $res);

        //First populate with logos from OMV repo
        $baseUrl = "http://openmediavault-plugin-developers.github.io/" .
            "openmediavault-docker-repo/";
        $imgBase = $baseUrl . "logos/";
        $repoUrl = $baseUrl . "dockerlist.xml";
        $data = file_get_contents($repoUrl);
        $xml = simplexml_load_string($data);
        foreach ($xml as $image) {
            //Use blank logo if no imageurl is specified
            if (strlen($image->imageurl) !== 0) {
                copy($imgBase . $image->imageurl, $logoPath . $image->imageurl);
            }
        }

        //Next populate with linuxserver.io logos
        $url = "http://tools.linuxserver.io/docker-templates.json";
        $local_logo_path = "/var/www/openmediavault/images/dockerrepo/";
        $json = file_get_contents($url);
        $data = json_decode($json);
        foreach ($data->applist as $image) {
            $imgRepo = trim((string)$image->Repository);
            if (strlen($image->Icon) !== 0) {
                $logo_url = trim((string)$image->Icon);
                $repo_name = trim((string)$image->Repository);
                preg_match('/^.*(\.[^\.]+)$/', $logo_url, $matches);
                $logo_suffix = $matches[1];
                $logo_name = str_replace("/", "_", $repo_name) . $logo_suffix;
                $new_hash = trim((string)$image->Icon_SHA);
                $local_hash_path = $local_logo_path . str_replace("/", "_", $repo_name) . ".sha";
                $old_hash = "";
                if (file_exists($local_hash_path)) {
                    $old_hash = trim(file_get_contents($local_hash_path));
                }
                if (strcmp($new_hash, $old_hash) !== 0) {
                    $imagick = new Imagick($logo_url);
                    $imagick->scaleImage(40, 40, true);
                    $imagick->modulateImage(100, 0, 100);
                    $imagick->writeImage($local_logo_path . $logo_name);
                    $imagick->clear();
                    file_put_contents($local_hash_path, $new_hash);
                }
            }
        }
    }

    /**
     * Retrieve list of Docker repos from Github source
     *
     * @param array  $params  An associative array with all RPC call parameters
     * @param string $context The context of the user maing the RPC call
     *
     * @return array $objects An associative array with all repos
     */
    public function getDockerRepo($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, "rpc.docker.executecommand");

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $settings = $db->get('conf.service.docker');

        $pulledImages = [];
        $cmd = "docker images 2>&1";
        OMVModuleDockerUtil::exec($cmd, $out, $res);
        for ($i = 1; $i < count($out); $i++) {
            preg_match('/^([\S]+)\s+\S+\s+([\S]+).*$/', $out[$i], $matches);
            $imgName = $matches[1];
            $pulledImages[trim($imgName)] = $matches[2];
        }
        $logoPath = "/var/www/openmediavault/images/dockerrepo/";
        $cmd = "mkdir -p $logoPath";
        OMVUtil::exec($cmd, $out, $res);

        //First populate with images from OMV repo
        $baseUrl = "http://openmediavault-plugin-developers.github.io/" .
            "openmediavault-docker-repo/";
        $imgBase = $baseUrl . "logos/";
        $repoUrl = $baseUrl . "dockerlist.xml";
        $data = file_get_contents($repoUrl);
        $xml = simplexml_load_string($data);
        $objects = [];
        foreach ($xml as $image) {
            //Use blank logo if no logo is found
            $logo = "";
            if (file_exists($logoPath . $image->imageurl)) {
                $logo = $image->imageurl;
            }
            //Silently skip any records that are missing data
            if ((strlen($image->name) === 0) || (strlen($image->desc) === 0) || (strlen($image->category) === 0) || (strlen($image->repo) === 0)) {
                continue;
            } else {
                $ports_ary = [];
                $pulled = false;
                $imgRepo = trim((string)$image->repo);
                if (isset($pulledImages[$imgRepo])) {
                    $pulled = true;
                    $image_obj = OMVModuleDockerUtil::getImage(
                        $pulledImages[$imgRepo],
                        $settings->get('apiPort')
                    );
                    $ports_ary = $image_obj->getPorts();
                }
                $temp = [
                    "name" => $image->name,
                    "category" => $image->category,
                    "logo" => $logo,
                    "desc" => $image->desc,
                    "repo" => $image->repo,
                    "pulled" => $pulled,
                    "timesync" => true,
                    "restartpolicy" => "always",
                    "privileged" => false,
                    "cenvvars" => [],
                    "envvars" => [],
                    "ports" => $ports_ary,
                    "portbindings" => [],
                    "extraargs" => "",
                    "isupdated" => false
                ];
                array_push($objects, $temp);
            }
        }

        //Next populate with linuxserver.io images
        $url = "http://tools.linuxserver.io/docker-templates.json";
        $local_logo_path = "/var/www/openmediavault/images/dockerrepo/";
        $json = file_get_contents($url);
        $data = json_decode($json);
        foreach ($data->applist as $image) {
            $cenv_ary = [];
            $ports_ary = [];
            $pulled = false;
            $updated = false;
            $imgRepo = trim((string)$image->Repository);
            if (isset($pulledImages[$imgRepo])) {
                $pulled = true;
                $image_obj = OMVModuleDockerUtil::getImage(
                    $pulledImages[$imgRepo],
                    $settings->get('apiPort')
                );
                $ports_ary = $image_obj->getPorts();
                $local_time = (int)date("U", $image_obj->getTimestamp());
                $repo_time = (int)date("U", trim((string)$image->Updated));
                if ($local_time < $repo_time) {
                    $updated = true;
                }
            }
            $timesync = true;
            if (strcmp(trim((string)$image->BindTime), "false") === 0) {
                $timesync = false;
            }
            $privileged = false;
            if (strcmp(trim((string)$image->Privileged), "true") === 0) {
                $privileged = true;
            }
            foreach ($image->Environment->Variable as $cenvvar) {
                if ((strcmp(trim((string)$cenvvar->Name), "PUID") === 0) ||
                    (strcmp(trim((string)$cenvvar->Name), "PGID") === 0)) {
                    continue;
                }
                $cenv_ary[$cenvvar->Name] = $cenvvar->Value;
            }
            $cenv_ary["PUID"] = 65534;
            $cenv_ary["PGID"] = 65534;
            $nw_mode = ucfirst(trim((string)$image->Networking->Mode));
            $port_bindings = [];
            if (strcmp($nw_mode, "Bridge") === 0) {
                foreach ($image->Networking->Publish[0]->Port as $port) {
                    array_push(
                        $port_bindings,
                        [
                            "containerportstring" => trim((string)$port->ContainerPort) . "/" . trim((string)$port->Protocol),
                            "containerportnr" => trim((string)$port->ContainerPort),
                            "hostip" => "0.0.0.0",
                            "hostport" => trim((string)$port->HostPort),
                            "proto" => "Select"
                        ]);

                }
            }
            $extraargs = "";
            if ((isset($image->ExtraParams)) && is_array($image->ExtraParams)) {
                foreach ($image->ExtraParams as $param) {
                    $extraargs .= trim((string)$param) . " ";
                }
            }
            $extraargs = trim($extraargs);
            $categories = explode(":", trim((string)$image->Category));
            $categories = implode("<br>", $categories);

            //Use blank logo if no Icon is specified
            $logo = "";
            if (strcmp(trim((string)$image->Icon), "") !== 0) {
                $logo_url = trim((string)$image->Icon);
                $repo_name = trim((string)$image->Repository);
                preg_match('/^.*(\.[^\.]+)$/', $logo_url, $matches);
                $logo_suffix = $matches[1];
                $logo_name = str_replace("/", "_", $repo_name) . $logo_suffix;
                if (file_exists($local_logo_path . $logo_name)) {
                    $logo = $logo_name;
                }
            }
            $desc = "No description available";
            if (isset($image->Overview)) {
                $desc = trim((string)$image->Overview);
            }

            $temp = [
                "name" => trim((string)$image->Name),
                "category" => $categories,
                "logo" => $logo,
                "desc" => $desc,
                "repo" => trim((string)$image->Repository),
                "pulled" => $pulled,
                "timesync" => $timesync,
                "restartpolicy" => "always",
                "privileged" => $privileged,
                "cenvvars" => $cenv_ary,
                "envvars" => [],
                "ports" => $ports_ary,
                "networkmode" => $nw_mode,
                "portbindings" => $port_bindings,
                "extraargs" => $extraargs,
                "isupdated" => $updated
            ];
            array_push($objects, $temp);
        }
        return $objects;
    }
}
