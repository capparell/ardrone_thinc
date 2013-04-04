// ros
#include "ros/ros.h"
#include "sensor_msgs/Image.h"
#include "std_msgs/Empty.h"
#include "std_srvs/Empty.h"
#include "geometry_msgs/Twist.h"

// opencv2
#include "sensor_msgs/image_encodings.h"
#include "image_transport/image_transport.h"
#include "cv_bridge/cv_bridge.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

// C++
#include <cstdlib>

// ardrone_autonomy
#include "ardrone_autonomy/CamSelect.h"
#include "ardrone_autonomy/Navdata.h"

// ardrone_thinc
#include "ArdroneThinc.hpp"

using namespace cv;
using namespace std;

int main(int argc, char *argv[]) {
    ros::init(argc, argv, "thinc_main");
    ros::NodeHandle n;
    ros::Rate loop_rate(10);
   
    if (argc < 3) {
        cout << "Arguments should include the following: number of columns, number of rows, then proceed in triples of drone id number, x coordinate, and y coordinate." << endl; 
        exit(0); 
    }

    //first two arguments are columns and rows, respectively.
    int c, r;
    int x_w, y_w; 
    c = atoi(argv[1]); 
    r = atoi(argv[2]); 
    
    ArdroneThinc at; 
    at.columns = c; 
    at.rows = r;

    ros::AsyncSpinner spinner((argc-3)/3);

    //after columns and rows, arguments proceed as follows: 
    //drone name, spawn x position, spawn y position, and
    //repeat for n drones
    for (int i = 0; i < (argc-3)/3; i++) {
        int id, x, y;
        string id_string = argv[3 + 3*i];
        string x_string = argv[4 + 3*i];
        string y_string = argv[5 + 3*i];

        id = atoi(id_string.c_str()); 
        x = atoi(x_string.c_str()); 
        y = atoi(y_string.c_str()); 

        drone* d = new drone(id, x, y);
        at.drones.push_back(d);

        //push publishers into vectors
        ros::Publisher launch, land, reset, twist, thresh;
        ros::Subscriber cam, navdata; 
        ros::ServiceServer waypoint;
        ros::ServiceClient camchannel, flattrim; 
        at.launch_publishers.push_back(launch); 
        at.land_publishers.push_back(land); 
        at.reset_publishers.push_back(reset); 
        at.twist_publishers.push_back(twist); 
        at.thresh_publishers.push_back(thresh); 
        at.cam_subscribers.push_back(cam); 
        at.navdata_subscribers.push_back(navdata);
        at.camchannel_clients.push_back(camchannel); 
        at.flattrim_clients.push_back(flattrim); 
        at.waypoint_navigator_services.push_back(waypoint); 

        //advertise
        at.launch_publishers[id] = n.advertise<std_msgs::Empty> 
            ("drone" + id_string + "/ardrone/takeoff", 5); 
        at.land_publishers[id] = n.advertise<std_msgs::Empty>
            ("drone" + id_string + "/ardrone/land", 5);
        at.reset_publishers[id] = n.advertise<std_msgs::Empty>
            ("drone" + id_string + "/ardrone/reset", 5);
        at.twist_publishers[id] = n.advertise<geometry_msgs::Twist>
            ("drone" + id_string + "/cmd_vel", 10);
        at.thresh_publishers[id] = n.advertise<sensor_msgs::Image>
            ("drone" + id_string + "/thinc/thresh", 10);
        at.navdata_subscribers[id] = n.subscribe<ardrone_autonomy::Navdata>
            ("drone" + id_string + "/ardrone/navdata", 1,
            &ArdroneThinc::NavdataCallback, &at);
        at.camchannel_clients[id] = n.serviceClient
            <ardrone_autonomy::CamSelect>
            ("drone" + id_string + "/ardrone/setcamchannel");
        at.flattrim_clients[id] = n.serviceClient<std_srvs::Empty>
            ("drone" + id_string + "/ardrone/flattrim");
        at.waypoint_navigator_services[id] = n.advertiseService
            ("Waypoint_Navigator_" + id_string, 
            &ArdroneThinc::Waypoint_Navigator_Callback, &at);  
    }

    at.cam_subscribers[0] = n.subscribe<sensor_msgs::Image>
        ("drone0/ardrone/bottom/ardrone/bottom/image_raw", 1,
        &ArdroneThinc::CamCallback0, &at);
    at.cam_subscribers[1] = n.subscribe<sensor_msgs::Image>
        ("drone1/ardrone/bottom/ardrone/bottom/image_raw", 1, 
        &ArdroneThinc::CamCallback1, &at); 

    // sleep to allow everything to register with roscore
    ros::Duration(1.0).sleep();

    // set camchannel on drone and takeoff
    if(ros::ok()) {
        // set camera to bottom
        ardrone_autonomy::CamSelect camsrv;
        camsrv.request.channel = 1;
        // calibrate to flat surface
        std_srvs::Empty trimsrv;
        // hover initially
        at.twist_msg.linear.x = 0;
        at.twist_msg.linear.y = 0;
        at.twist_msg.linear.z = 0;
        cout << "taking off..." << endl; 
        
        for (int i = 0; i < at.drones.size(); i++) {
            at.camchannel_clients[i].call(camsrv); 
            at.flattrim_clients[i].call(trimsrv); 
            at.launch_publishers[i].publish(at.empty_msg); 
        }
    } 
    
    spinner.start();
    ros::waitForShutdown();

    return 0;
}
