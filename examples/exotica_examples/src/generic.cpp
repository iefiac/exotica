#include <exotica/Exotica.h>

using namespace exotica;

void run()
{
    ros::NodeHandle nhg_;

    // Scene using joint group 'arm'
    Initializer scene("Scene",{{"Name",std::string("MyScene")},{"JointGroup",std::string("arm")}});
    // End-effector task map with two position frames
    Initializer map("exotica/EffPosition",{
                        {"Name",std::string("Position")},
                        {"Scene",std::string("MyScene")},
                        {"EndEffector",std::vector<Initializer>({
                             Initializer("Frame",{{"Link",std::string("lwr_arm_6_link")}}),
                         })}});
    Eigen::VectorXd W(7);
    W << 7,6,5,4,3,2,1;

    Initializer problem("exotica/UnconstrainedEndPoseProblem",{
                            {"Name",std::string("MyProblem")},
                            {"PlanningScene",scene},
                            {"Maps",std::vector<Initializer>({map})},
                            {"W",W},
                            {"Tolerance",1e-5},
                        });

    Initializer solver("exotica/IKsolver",{
                           {"Name",std::string("MySolver")},
                           {"MaxIt",1},
                           {"MaxStep", 0.1},
                           {"C",1e-3},
                       });

    HIGHLIGHT_NAMED("GenericLoader","Loaded from a hardcoded generic initializer.");

    // Initialize

    PlanningProblem_ptr any_problem = Setup::createProblem(problem);
    MotionSolver_ptr any_solver = Setup::createSolver(solver);

    // Assign the problem to the solver
    any_solver->specifyProblem(any_problem);
    UnconstrainedEndPoseProblem_ptr my_problem = boost::static_pointer_cast<UnconstrainedEndPoseProblem>(any_problem);

    // Create the initial configuration
    Eigen::VectorXd q = Eigen::VectorXd::Zero(any_problem->getScene()->getNumJoints());
    Eigen::MatrixXd solution;


    ROS_INFO_STREAM("Calling solve() in an infinite loop");

    double t = 0.0;
    ros::Rate loop_rate(500.0);
    ros::WallTime init_time = ros::WallTime::now();

    while (ros::ok())
    {
        ros::WallTime start_time = ros::WallTime::now();

        // Update the goal if necessary
        // e.g. figure eight
        t = ros::Duration((ros::WallTime::now() - init_time).toSec()).toSec();
        my_problem->y << 0.6,
                -0.1 + sin(t * 2.0 * M_PI * 0.5) * 0.1,
                0.5 + sin(t * M_PI * 0.5) * 0.2;

        // Solve the problem using the IK solver
        any_solver->Solve(q, solution);

        double time = ros::Duration((ros::WallTime::now() - start_time).toSec()).toSec();
        ROS_INFO_STREAM_THROTTLE(0.5,
                                 "Finished solving in "<<time<<"s. Solution ["<<solution<<"]");
        q = solution.row(solution.rows() - 1);

        my_problem->Update(q);
        my_problem->getScene()->getSolver().publishFrames();

        ros::spinOnce();
        loop_rate.sleep();
    }

    // All classes will be destroyed at this point.
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "ExoticaManualInitializationExampleNode");
    ROS_INFO_STREAM("Started");

    // Run demo code
    run();

    // Clean up
    // Run this only after all the exoica classes have been disposed of!
    Setup::Destroy();
}
