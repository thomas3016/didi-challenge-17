#include <ros/ros.h>
#include <visualization_msgs/Marker.h>
#include <visualization_msgs/MarkerArray.h>


#include <iostream>
#include <vector>

#include <sensor_msgs/Image.h>
#include <perception_msgs/ObstacleList.h>


using namespace std;

const float framerate_camera=2.4;
double last_interpolated=0;
vector<ros::Time> camera_timestamps;
ros::Publisher interpolated_pub,marker_pub;

bool online,marker_visualization;
perception_msgs::ObstacleList last_detected;
vector<int> matched_indices;

//array of interpolation coefficients
vector<vector<double>> matched_a,matched_b; //x,y,z,alpha, height

visualization_msgs::Marker sphere_marker;
visualization_msgs::MarkerArray marker_array;


void get_linear_coefficients(const double &time0,const double &time1,const float &y0,const float &y1, double &a,double &b){

    //cout<< time0<<" "<< time1<<" "<< y0<<" "<< y1<<" "<<a<<" "<<b<<endl;

    //y=ax+b

    a=(y1-y0)/(time1-time0);
    b=y0-time0*(y1-y0)/(time1-time0);
}

void camera_sub_callback(sensor_msgs::ImageConstPtr msg){
    //std::cout << std::setprecision(14)<<" Adding camera_timestamp: "<<msg->header.stamp.toSec()<<endl;
    camera_timestamps.push_back(msg->header.stamp);

    if(online==true){
        perception_msgs::ObstacleList interpolated;
        interpolated.header.stamp=msg->header.stamp;


        //CONTINUE HERE
        for(size_t i=0;i<matched_indices.size();i++){
            perception_msgs::Obstacle temp;
            //cout<<setprecision(14)<<useful_timestamps[k]<<"     ";

            temp=last_detected.obstacles[matched_indices[i]];
            //ros::Time::fromSec(useful_timestamps[k]) stamp;
            temp.header.stamp.fromSec(msg->header.stamp.toSec());

            // extrapolate using the current camera timestamp as x point and the linear coefficients calculated before
            temp.location.x=matched_a[i][0]*msg->header.stamp.toSec()+matched_b[i][0];
            temp.location.y=matched_a[i][1]*msg->header.stamp.toSec()+matched_b[i][1];
            temp.location.z=matched_a[i][2]*msg->header.stamp.toSec()+matched_b[i][2];
            temp.alpha=matched_a[i][3]*msg->header.stamp.toSec()+matched_b[i][3];
            temp.height=matched_a[i][4]*msg->header.stamp.toSec()+matched_b[i][4];
            interpolated.obstacles.push_back(temp);

            //cout<<setprecision(14)<<ros::Time::now()<<endl;

            std::cout << std::setprecision(14)<<" Original Time: "<<last_detected.header.stamp.toSec()<<" X "<<last_detected.obstacles[matched_indices[i]].location.x<<" Y "<<last_detected.obstacles[matched_indices[i]].location.y<<" alpha "<<last_detected.obstacles[matched_indices[i]].alpha<<" height "<<last_detected.obstacles[matched_indices[i]].height<<"\n";
            std::cout << std::setprecision(14)<<" Extrapolation Time: "<<temp.header.stamp.toSec()<<" X "<<temp.location.x<<" Y "<<temp.location.y<<" alpha "<<temp.alpha<<" height "<<temp.height<<"\n\n";

        }
        interpolated.header.stamp=msg->header.stamp;
        interpolated_pub.publish(interpolated);

    }
}


void detection_sub_callback(const perception_msgs::ObstacleListConstPtr msg){


    static bool first_time=true;
    //std::cout << std::setprecision(14)<<" Birdview camera_timestamp: "<<msg->header.stamp.toSec()<<endl;


    if(first_time==true){

        if(online==true){
            matched_a.clear();
            matched_b.clear();
            matched_indices.clear();

        }


        //vector to put inside the timestamps which are between the 2 interpolation points
        vector<ros::Time> useful_timestamps;

        //checking which ones are between them, erasing the earliers, if any

        vector<ros::Time> temp_timestamps;
        for(int i=0;i<camera_timestamps.size();i++){
            if(last_detected.header.stamp<camera_timestamps[i]&&camera_timestamps[i]<msg->header.stamp){
                //cout<<"useful "<<setprecision(14)<<camera_timestamps[i]<<"     ";
                useful_timestamps.push_back(camera_timestamps[i]);
                //camera_timestamps.erase(camera_timestamps.begin()+i);
            }
            //            else if(camera_timestamps[i]<last_detected.header.stamp.toSec()){
            //                camera_timestamps.erase(camera_timestamps.begin()+i);
            //            }
            if(camera_timestamps[i]>msg->header.stamp)
                temp_timestamps.push_back(camera_timestamps[i]);
        }
        camera_timestamps=temp_timestamps;



        //one interpolation for every cameratimestamp, --> one publised message
        for(size_t k=0;k<useful_timestamps.size();k++){
            perception_msgs::ObstacleList interpolated;

            //clear previus markers
            marker_array.markers.clear();



            size_t i(0),j(0);

            //checking id coincidences between last message and current one, interpolate between the ones that match
            for( i=0;i<msg->obstacles.size();i++){
                bool match=false;






                for (j=0;j<last_detected.obstacles.size();j++){
                    if(last_detected.obstacles[j].id==msg->obstacles[i].id){
                        //std::cout <<"MATCH in: "<<last_detected.obstacles[j].id<<" =" <<msg->obstacles[i].id<<endl;
                        match=true;
                        break;
                    }
                }

                if(match==true){
                    //DO INTERPOLATION HERE



                    vector<double> a,b;
                    a.resize(5,0);
                    b.resize(5,0);

                    //get linear coefficients to interpolate the desired values
                    get_linear_coefficients(last_detected.header.stamp.toSec(),msg->header.stamp.toSec(),last_detected.obstacles[j].location.x,msg->obstacles[i].location.x,a[0],b[0]);
                    get_linear_coefficients(last_detected.header.stamp.toSec(),msg->header.stamp.toSec(),last_detected.obstacles[j].location.y,msg->obstacles[i].location.y,a[1],b[1]);
                    get_linear_coefficients(last_detected.header.stamp.toSec(),msg->header.stamp.toSec(),last_detected.obstacles[j].location.z,msg->obstacles[i].location.z,a[2],b[2]);
                    get_linear_coefficients(last_detected.header.stamp.toSec(),msg->header.stamp.toSec(),last_detected.obstacles[j].alpha,msg->obstacles[i].alpha,a[3],b[3]);
                    get_linear_coefficients(last_detected.header.stamp.toSec(),msg->header.stamp.toSec(),last_detected.obstacles[j].height,msg->obstacles[i].height,a[4],b[4]);


                    if(online==false){
                        std::cout << std::setprecision(14)<<" interpolating from Time: "<<last_detected.header.stamp.toSec()<<" X "<<last_detected.obstacles[j].location.x<<" Y "<<last_detected.obstacles[j].location.y<<" Alpha "<<last_detected.obstacles[j].alpha<<" Height "<<last_detected.obstacles[j].height<<"\n";
                        //std::cout <<" timestamps between: "<<useful_timestamps.size()<<endl;



                        perception_msgs::Obstacle temp;

                        //cout<<setprecision(14)<<useful_timestamps[k]<<"     ";

                        temp=msg->obstacles[i];
                        temp.header.stamp=useful_timestamps[k];

                        // interpolate using the useful_timestamps as x point and the linear coefficients calculated before
                        temp.location.x=a[0]*useful_timestamps[k].toSec()+b[0];
                        temp.location.y=a[1]*useful_timestamps[k].toSec()+b[1];
                        temp.location.z=a[2]*useful_timestamps[k].toSec()+b[2]-1.72;
                        temp.alpha=a[3]*useful_timestamps[k].toSec()+b[3];
                        temp.height=a[4]*useful_timestamps[k].toSec()+b[4];

                        interpolated.obstacles.push_back(temp);

                        //marker visualization
                        if(marker_visualization==true){
                                float sphere_size=max(max(temp.height,temp.width),temp.length);
                                sphere_marker.header.stamp=useful_timestamps[k];
                                sphere_marker.scale.x=sphere_size;
                                sphere_marker.scale.y=sphere_size;
                                sphere_marker.scale.z=sphere_size;

                                sphere_marker.pose.position.x=temp.location.x;
                                sphere_marker.pose.position.y=temp.location.y;
                                sphere_marker.pose.position.z=temp.location.z-1.72;

                                marker_array.markers.push_back(sphere_marker);

                                //   sphere.

                        }

                        //cout<<setprecision(14)<<ros::Time::now()<<endl;

                        std::cout << std::setprecision(14)<<" Interpolation Time: "<<temp.header.stamp.toSec()<<" X "<<temp.location.x<<" Y "<<temp.location.y<<" alpha "<<temp.alpha<<" height "<<temp.height<<"\n";


                        std::cout << std::setprecision(14)<<" to Time: "<<msg->header.stamp.toSec()<<" X "<<msg->obstacles[i].location.x<<" Y "<<msg->obstacles[i].location.y<<" alpha "<<msg->obstacles[i].alpha<<" height "<<msg->obstacles[i].height<<"\n\n";
                    }


                    else{
                        matched_a.push_back(a);
                        matched_b.push_back(b);
                        matched_indices.push_back(i);
                    }


                }
                else{
                    cout<<"no match\n";
                }

            }
            cout<<"_ _ _ _ _ \n";

            if(online==false){
                //what timestamp should this have? the individual obstacles have the camera one
                interpolated.header.stamp=useful_timestamps[k];
                interpolated_pub.publish(interpolated);
            }



            //publish the marker
            marker_pub.publish(marker_array);


        }
    }

    else
    {
        first_time=false;
    }
    //update the starting point of the interpolation
    last_detected=*msg;
    //clear the interpolation points (have already been used)
    //camera_timestamps.clear();

    //std::cout <<"publishing "<<interpolated.obstacles.size()<<"..\n\n\n";




}



int main(int argc, char* argv[]){

    ros::init(argc,argv,"interpolation");
    ros::NodeHandle public_nh, private_nh("~");


    string camera_sub_topic, detection_sub_topic,interpolated_pub_topic,markers_pub_topic;

    private_nh.param<string>("detection_sub_topic",detection_sub_topic,"cnn/filtered_detection");
    private_nh.param<string>("camera_sub_topic",camera_sub_topic,"kitti/camera_color_left/image_raw");
    private_nh.param<string>("interpolated_pub_topic",interpolated_pub_topic,"interpolated_detections");
    private_nh.param<string>("markers_pub_topic",markers_pub_topic,"markers");
    private_nh.param("online",online,false);
    private_nh.param("marker_visualization",marker_visualization,true);



    if(marker_visualization==true){
        sphere_marker.ns="detections";
        sphere_marker.header.frame_id="velodyne";
        sphere_marker.id=0;
        sphere_marker.type=visualization_msgs::Marker::SPHERE;
        sphere_marker.color.r=0;
        sphere_marker.color.g=1;
        sphere_marker.color.b=0;
        sphere_marker.color.a=0.5;
        marker_pub=public_nh.advertise<visualization_msgs::MarkerArray>(markers_pub_topic,1);
    }






    ros::Subscriber detection_sub,camera_sub;
    interpolated_pub;

    interpolated_pub=public_nh.advertise<perception_msgs::ObstacleList>(interpolated_pub_topic,1);


    detection_sub=public_nh.subscribe(detection_sub_topic,0,detection_sub_callback);
    camera_sub=public_nh.subscribe(camera_sub_topic,10,camera_sub_callback);

    while (ros::ok())
    {


        ros::spinOnce();
    }

    return 0;
}
