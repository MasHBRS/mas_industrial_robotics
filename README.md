This repository contains most of the code used by our team on the youBot for the [RoboCup@Work](https://atwork.robocup.org/) competition. In the competition, the robot should transport industrial objects such as metal profiles, nuts and bolts, tools, etc. between workstations based on a given task order. This requires the robot to autonomously navigate in a known environment with previously unseen obstacles, recognize and grasp small and textureless objects, grasp objects from a rotating table, insert objects precisely into cavities and plan the most efficient sequence of actions to complete the task.

Several other repositories are used; the full list can be found in [repository.rosinstall](repository.rosinstall). The main ones are:

* [mas_common_robotics](https://github.com/b-it-bots/mas_common_robotics): robot-independent code that is sometimes shared with our other robots
* [mas_navigation](https://github.com/b-it-bots/mas_navigation): robot-independent navigation components such as the force-field recovery behaviour for move base, direct base controller, etc.
* [mir_perception_models](https://github.com/b-it-bots/mir_perception_models): trained neural network models for perception
* [technical_drawings](https://github.com/b-it-bots/technical_drawings): CAD models for various parts used on the youBot.
* [gripper_controller](https://github.com/b-it-bots/youbot_dynamixel_gripper_controller): code for the OpenRB-150 board to control the gripper using two Dynamixel motors.

<p align="center">
  <img src="docs/source/images/youbot_annotated.png" width="90%" />
</p>

## Install Ubuntu
The repository and its related components have been tested under the following Ubuntu distributions:

- ROS Kinetic: Ubuntu 16.04
- ROS Melodic: Ubuntu 18.04
- ROS Noetic: Ubuntu 20.04

If you do not have a Ubuntu distribution on your computer you can download it here

     http://www.ubuntu.com/download

## Git - Version Control
### Install Git Software
Install the Git core components and some additional GUI's for the version control:

     sudo apt-get install git-core gitg gitk

### Set Up Git
Now it's time to configure your settings. To do this you need to open a new Terminal. First you need to tell git your name, so that it can properly label the commits you make:

     git config --global user.name "Your Name Here"

Git also saves your email address into the commits you make.

     git config --global user.email "your-email@youremail.com"


### GIT Tutorial
If you have never worked with git before, we recommend to go through the following basic git tutorial:

     http://excess.org/article/2008/07/ogre-git-tutorial/

## Docker (Recommended, Optional)
### Getting started with docker
The latest versions of docker-engine and docker-compose have to be installed before getting started. Please have a look at [docker's official website](https://docs.docker.com/get-started/overview/) for more insights into the working and usage of docker images and docker containers.

The docker images available [here](https://github.com/orgs/b-it-bots/packages) provide a proper development environment -with ros pre-installed and without any missing dependencies- for the MAS industrial software. It is highly recommended that you use docker containers to build and run your nodes rather than directly installing ROS and working with the MAS industrial software on your PC.

First, you need to pull the corresponding image you want to use:
```bash
#kinetic image
docker pull  ghcr.io/b-it-bots/mas_industrial_robotics/industrial-kinetic:latest

#melodic image
docker pull ghcr.io/b-it-bots/mas_industrial_robotics/industrial-melodic:latest
```

The main branch of the ros distro should always reflect `latest` tag in the github registry, for example `kinetic` branch reflects `mas_industrial_robotics/industrial-kinetic:latest`. 
The ros distro `devel` branch always reflects `devel` tag, for example `kinetic` branch reflects `mas_industrial_robotics/industrial-kinetic:devel`

Start container with `docker-compose`:
```bash
docker-compose -f start-container.yaml up <industrial_kinetic|industrial_melodic>
```

Log in to the container:
```bash
docker exec -it mas_industrial_robotics_industrial_kinetic_1 /bin/bash
```

More detailed tutorials on how to use MAS Industrial Robotics softwares with docker are available [here](https://b-it-bots.readthedocs.io/en/melodic/docker.html)

## ROS - Robot Operating System
### Install ROS
The repository has been tested successfully with the following ROS distributions. Use the link behind a ROS distribution to get to the particular ROS installation instructions.
Alternatively, you can skip this step, as ROS Melodic is automatically installed by the setup.sh script described in this [section](#Clone-and-compile-the-MAS-industrial-robotics-software).


- ROS Noetic - http://wiki.ros.org/noetic/Installation/Ubuntu

NOTE: Do not forget to update your .bashrc!


### ROS Tutorials
If you have never worked with ROS before, we recommend to go through the beginner tutorials provided by ROS:

     http://wiki.ros.org/ROS/Tutorials

In order to understand at least the different core components of ROS, you have to start from tutorial 1 ("Installing and Configuring Your ROS Environment") till tutorial 7 ("Understanding ROS Services and Parameters").


## Clone and compile the MAS industrial robotics software
First of all you have to clone the repository.

    mkdir ~/temp
    cd ~/temp
    git clone --branch noetic git@github.com:b-it-bots/mas_industrial_robotics.git

Navigate into the cloned repository and run setup.sh file.

     ./setup.sh --ros_install <full|base> --ros_distro <melodic|kinetic|noetic> --ws_dir <$HOME/catkin_ws> --docker 0

Example:

     ./setup.sh --ros_install full --ros_distro noetic --ws_dir $HOME/catkin_ws --docker 0

**Note:** In case you are using the docker images, please pay attention to the mounted directory path in the container. All the above paths should be relative to your mounted folder inside the docker container and not your local file system.

This script does the following,

* installs ROS, if not previously installed, provided the optional argument full is given. In case, you dont want to install ROS then replace full by none.
* creates a catkin workspace folder in the directory specified in the argument or by default places it in home directory, i.e. ~/catkin_ws (if it does not exist)
* clones the mas_industiral_robotics repository along with other repositories mentioned in repository.rosinstall file to \<your folder\>/catkin_ws/src and installs the necessary ros dependencies
* initiates a catkin build in the catkin workspace

**Note:** A catkin_ws built inside docker container can be built only within docker containers having the same mounted directory in the container. Do not try to switch the catkin_ws build between docker containers and local PC, as it will produce errors due to conflicting paths.

Add the following to your bashrc and source your bashrc, so that you need not execute ./setup.sh script each time you open your terminal

     source <your folder>/catkin_ws/devel/setup.bash

If no errors appear everything is ready to use. Great job!

Finally delete the initially cloned mas_industrial_robotics in ~/temp, as all necessary repositories and dependencies are successfully cloned and installed in your catkin workspace

     rm -rf ~/temp/mas_industrial_robotics


### Setting the Environment Variables
#### ROBOT variable
With the ROBOT variable you can choose which hardware configuration should be loaded when starting the robot. The following line will add the variable to your .bashrc:

     echo "export ROBOT=youbot-brsu-4" >> ~/.bashrc
     source ~/.bashrc



#### ROBOT_ENV Variable
The ROBOT_ENV variable can be used to switch between different environments. The following line will add the variable to your .bashrc:
#### Real robot
     echo "export ROBOT_ENV=brsu-c025" >> ~/.bashrc
     source ~/.bashrc
#### Simulation
     echo "export ROBOT_ENV=brsu-c025-sim" >> ~/.bashrc
     source ~/.bashrc


## Bring up the robot and it's basic components
### In Simulation

     roslaunch mir_bringup_sim robot.launch
	     
### At the Real Robot

     roslaunch mir_bringup robot.launch


## Test the base

     roslaunch mir_teleop teleop_keyboard.launch


## Visualize the robot state and sensor data

     rosrun rviz rviz


## Build a map for base navigation

     roslaunch mir_2dslam 2dslam.launch


## Use autonomous navigation
### Omni-directional navigation

     roslaunch mir_2dnav 2dnav.launch nav_mode:=dwa

Click on the menu bar "File -> Open Config", navigate to "~/indigo/src/mas_industrial_robotics" and select the "youbot.rviz" file.

# Note on Contributions:

[Pre-commit](https://pre-commit.com/#intro) hooks has been added to this repository. Please note that you will not be able to locally commit your changes to git until all the checks in the .pre-commit-config.yaml pass. Although, cpp and python code formatters are present in .pre-commit-config.yaml, some serious violations of the standard coding guidelines will not be automatically fixed while running the pre-commit hooks. These errors will be displayed while running git commit and have to be manually fixed. Users will not be able to commit their code, until these errors are fixed. Alternatively, one could also verify if the pre-commit hooks pass before actually committing the code to git. To do so please run the following command after making necessary changes to your code.
```
pre-commit run --all-files
```
