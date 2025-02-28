#!/usr/bin/python

import rospy
import smach
from actionlib import SimpleActionClient
from actionlib_msgs.msg import GoalStatus
from diagnostic_msgs.msg import KeyValue
from mir_actions.utils import Utils
from mir_planning_msgs.msg import GenericExecuteAction, GenericExecuteGoal


class place_object(smach.State):
    def __init__(self, platform_name):
        smach.State.__init__(self, outcomes=["success", "failed"])
        self.client = SimpleActionClient("place_object_server", GenericExecuteAction)
        self.client.wait_for_server()
        self.goal = GenericExecuteGoal()
        self.goal.parameters.append(
            KeyValue(key="location", value=str(platform_name).upper())
        )

    def execute(self, userdata):
        self.client.send_goal(self.goal)
        self.client.wait_for_result(rospy.Duration.from_sec(15.0))
        state = self.client.get_state()
        if state == GoalStatus.SUCCEEDED:
            return "success"
        else:
            return "failed"


class pick_object(smach.State):
    def __init__(self):
        smach.State.__init__(self, outcomes=["success", "failed"])
        self.client = SimpleActionClient("wbc_pick_object_server", GenericExecuteAction)
        self.client.wait_for_server()
        self.goal = GenericExecuteGoal()
        self.goal.parameters.append(KeyValue(key="object", value="any"))

    def execute(self, userdata):
        self.client.send_goal(self.goal)
        self.client.wait_for_result(rospy.Duration.from_sec(30.0))
        state = self.client.get_state()
        if state == GoalStatus.SUCCEEDED:
            return "success"
        else:
            return "failed"


class perceive_location(smach.State):
    def __init__(self, obj_category="atwork"):
        smach.State.__init__(self, outcomes=["success", "failed"])
        self.client = SimpleActionClient(
            "perceive_location_server", GenericExecuteAction
        )
        self.client.wait_for_server()
        self.goal = GenericExecuteGoal()
        self.goal.parameters.append(
            KeyValue(key="obj_category", value=str(obj_category)))

    def execute(self, userdata):
        self.client.send_goal(self.goal)
        self.client.wait_for_result(rospy.Duration.from_sec(30.0))
        state = self.client.get_state()
        if state == GoalStatus.SUCCEEDED:
            return "success"
        else:
            return "failed"


class move_base(smach.State):
    def __init__(self, destination_location):
        smach.State.__init__(self,
                             outcomes=["success", "failed"],
                             input_keys=["goal"])
        self.client = SimpleActionClient("move_base_safe_server", GenericExecuteAction)
        self.client.wait_for_server()
        self.goal = GenericExecuteGoal()
        self.destination_location = destination_location


    def execute(self, userdata):
        self.goal.parameters = []
        self.goal.parameters.append(
            KeyValue(key="arm_safe_position", value="barrier_tape")
        )
        # if location is sent as an argument to this state, set it here
        if self.destination_location is not None:
            self.goal.parameters.append(
                KeyValue(
                    key="destination_location", value=str(self.destination_location).upper()
                )
            )
        current_destination_location = Utils.get_value_of(self.goal.parameters, "destination_location")
        # if location has not  been set  read it from userdata (either
        # destination_location or location)
        if current_destination_location is None:
            location = Utils.get_value_of(userdata.goal.parameters, "destination_location")
            if location is None:
                location = Utils.get_value_of(userdata.goal.parameters, "location")
                if location is None:
                    rospy.logerr("Location not specified. Not calling move_base")
                    return 'failed'
                self.goal.parameters.append(
                    KeyValue(key="destination_location", value=str(location).upper())
                )

        self.client.send_goal(self.goal)
        self.client.wait_for_result(rospy.Duration.from_sec(int(15.0)))
        state = self.client.get_state()
        if state == GoalStatus.SUCCEEDED:
            return "success"
        else:
            return "failed"


class insert_object(smach.State):
    def __init__(self, robot_platform, container):
        smach.State.__init__(self, outcomes=["success", "failed"])
        self.client = SimpleActionClient("insert_object_server", GenericExecuteAction)
        self.client.wait_for_server()
        self.goal = GenericExecuteGoal()
        self.goal.parameters.append(
            KeyValue(key="robot_platform", value=str(robot_platform).upper())
        )
        self.goal.parameters.append(KeyValue(key="hole", value=str(container).upper()))

    def execute(self, userdata):
        self.client.send_goal(self.goal)
        self.client.wait_for_result(rospy.Duration.from_sec(int(30.0)))
        state = self.client.get_state()
        if state == GoalStatus.SUCCEEDED:
            return "success"
        else:
            return "failed"


class stage_object(smach.State):
    def __init__(self, platform=None):
        smach.State.__init__(self, outcomes=["success", "failed"], input_keys=["goal"])
        self.client = SimpleActionClient("stage_object_server", GenericExecuteAction)
        self.client.wait_for_server()
        self.goal = GenericExecuteGoal()
        self.platform = platform

    def execute(self, userdata):
        self.goal.parameters = []
        if self.platform is not None:
            self.goal.parameters.append(
                KeyValue(key="platform", value=str(self.platform).upper())
            )

        obj = Utils.get_value_of(userdata.goal.parameters, "peg")
        if obj is not None:
            self.goal.parameters.append(
                KeyValue(key="object", value=str(obj).upper())
            )

        # initialise platform in goal if not already initialised
        current_platform = Utils.get_value_of(self.goal.parameters, "platform")
        if current_platform is None:
            platform = Utils.get_value_of(userdata.goal.parameters, "platform")
            if platform is None:
                rospy.logwarn("Platform not provided. Using default")
                platform = "platform_middle"
            self.goal.parameters.append(
                KeyValue(key="platform", value=str(platform).upper())
            )

        self.client.send_goal(self.goal)
        self.client.wait_for_result(rospy.Duration.from_sec(30.0))
        state = self.client.get_state()
        if state == GoalStatus.SUCCEEDED:
            return "success"
        else:
            return "failed"


class unstage_object(smach.State):
    def __init__(self, platform=None):
        smach.State.__init__(self, outcomes=["success", "failed"], input_keys=["goal"])
        self.client = SimpleActionClient("unstage_object_server", GenericExecuteAction)
        self.client.wait_for_server()
        self.goal = GenericExecuteGoal()
        self.platform = platform

    def execute(self, userdata):
        self.goal.parameters = []
        if self.platform is not None:
            self.goal.parameters.append(
                KeyValue(key="platform", value=str(self.platform).upper())
            )

        obj = Utils.get_value_of(userdata.goal.parameters, "peg")
        if obj is not None:
            self.goal.parameters.append(
                KeyValue(key="object", value=str(obj).upper())
            )

        # initialise platform in goal if not already initialised
        current_platform = Utils.get_value_of(self.goal.parameters, "platform")
        if current_platform is None:
            platform = Utils.get_value_of(userdata.goal.parameters, "platform")
            if platform is None:
                rospy.logwarn("Platform not provided. Using default")
                platform = "platform_middle"
            self.goal.parameters.append(
                KeyValue(key="platform", value=str(platform).upper())
            )

        self.client.send_goal(self.goal)
        self.client.wait_for_result(rospy.Duration.from_sec(30.0))
        state = self.client.get_state()
        if state == GoalStatus.SUCCEEDED:
            return "success"
        else:
            return "failed"


class perceive_cavity(smach.State):
    def __init__(self, perception_mode='three_view'):
        smach.State.__init__(self, outcomes=["success", "failed"])
        self.client = SimpleActionClient("perceive_cavity_server", GenericExecuteAction)
        self.client.wait_for_server()
        self.goal = GenericExecuteGoal()
        self.goal.parameters.append(KeyValue(key='perception_mode', value=perception_mode))

    def execute(self, userdata):
        self.client.send_goal(self.goal)
        self.client.wait_for_result(rospy.Duration.from_sec(30.0))
        state = self.client.get_state()
        if state == GoalStatus.SUCCEEDED:
            return "success"
        else:
            return "failed"
