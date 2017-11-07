#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cmath>

#include <json/json.h>

#include "Catch.hpp"
#include "MatterSim.hpp"


using namespace mattersim;

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

// Note: tests tagged with 'Rendering' will require the dataset to be installed
// To run tests without the dataset installed:
//      $ ./build/tests exclude:[Rendering]


double radians(float deg) {
    return deg * M_PI / 180.f;
}

float heading[10] =     {  10,  350, 350,  1, 90, 180,   90,  270,   90, 270 };
float heading_chg[10] = { -20, -360, 371, 89, 90, -90, -180, -180, -180,   0 };
float elevation[10] =     {  10,   10, -26, -40, -40, -40,  50,  50,  40,  0 };
float elevation_chg[10] = {   0,  -36, -30, -10,   0,  90,   5, -10, -40,  0 };


TEST_CASE( "Simulator can start new episodes and do simple motion", "[Simulator]" ) {

    std::vector<std::string> scanIds {"2t7WUuJeko7", "17DRP5sb8fy"};
    std::vector<std::string> viewpointIds {"cc34e9176bfe47ebb23c58c165203134", "5b9b2794954e4694a45fc424a8643081"};
    Simulator sim;
    sim.setCameraResolution(200,100); // width,height
    sim.setCameraFOV(45); // 45deg vfov, 90deg hfov
    sim.setRenderingEnabled(false);
    CHECK(sim.setElevationLimits(radians(-40),radians(50)));
    REQUIRE_NOTHROW(sim.init());
    for (int i = 0; i < scanIds.size(); ++i) {
        std::string scanId = scanIds[i];
        std::string viewpointId = viewpointIds[i];
        REQUIRE_NOTHROW(sim.newEpisode(scanId, viewpointId, radians(heading[0]), radians(elevation[0])));
        for (int t = 0; t < 10; ++t ) {
            SimStatePtr state = sim.getState();
            CHECK( state->scanId == scanId );
            CHECK( state->step == t );
            CHECK( state->heading == Approx(radians(heading[t])) );
            CHECK( state->elevation == Approx(radians(elevation[t])) );
            CHECK( state->rgb.rows == 100 );
            CHECK( state->rgb.cols == 200 );
            CHECK( state->location->viewpointId == viewpointId );
            std::vector<ViewpointPtr> actions = state->navigableLocations;
            int ix = t % actions.size(); // select an action
            sim.makeAction(ix, radians(heading_chg[t]), radians(elevation_chg[t]));
            viewpointId = actions[ix]->viewpointId;
        }
    }
    REQUIRE_NOTHROW(sim.close());
}

TEST_CASE( "Simulator state->navigableLocations is correct", "[Simulator]" ) {

    std::vector<std::string> scanIds;
    std::ifstream infile ("./connectivity/scans.txt", std::ios_base::in);
    std::string scanId;
    while (infile >> scanId) {
        scanIds.push_back (scanId);
    }
    Simulator sim;
    sim.setCameraResolution(20,20); // don't really care about the image
    sim.setCameraFOV(90); // 90deg vfov, 90deg hfov
    double half_hfov = M_PI/4;
    sim.setRenderingEnabled(false);
    sim.setSeed(1);
    REQUIRE_NOTHROW(sim.init());
    for (auto scanId : scanIds) {
        REQUIRE_NOTHROW(sim.newEpisode(scanId)); // start anywhere, but repeatably so

        // Load connectivity graph
        Json::Value root;
        auto navGraphFile =  "./connectivity/" + scanId + "_connectivity.json";
        std::ifstream ifs(navGraphFile, std::ifstream::in);
        ifs >> root;
        // Find included viewpoints
        std::vector<bool> included;
        SimStatePtr state = sim.getState();
        for (auto viewpoint : root) {
            included.push_back(viewpoint["included"].asBool());
            if (viewpoint["image_id"].asString() == state->location->viewpointId) {
                INFO("Don't let newEpisode spawn at an excluded viewpoint");
                CHECK(included.back());
            }
        }

        // Check a short episode
        for (int t = 0; t < 10; ++t ) {
            state = sim.getState();
            CHECK( state->scanId == scanId );
            CHECK( state->step == t );

            // navigableLocations from sim into a map
            std::unordered_map <std::string, ViewpointPtr> locs;
            for (auto v : state->navigableLocations) {
                locs[v->viewpointId] = v;
            }

            // Find current viewpoint in json file
            Json::Value currentViewpoint;
            for (auto viewpoint : root) {
                auto viewpointId = viewpoint["image_id"].asString();
                if (viewpointId == state->location->viewpointId) {
                    currentViewpoint = viewpoint;
                    break;
                }
            }
            REQUIRE_FALSE(currentViewpoint.empty());

            // Check navigableLocations are correct
            int navigableCount = 0;
            for (int i = 0; i < included.size(); ++i) {
                std::string curr = currentViewpoint["image_id"].asString();
                std::string target = root[i]["image_id"].asString();
                // Read current position
                float x = currentViewpoint["pose"][3].asFloat();
                float y = currentViewpoint["pose"][7].asFloat();
                float z = currentViewpoint["pose"][11].asFloat();
                // Read target position
                float tar_x = root[i]["pose"][3].asFloat();
                float tar_y = root[i]["pose"][7].asFloat();
                float tar_z = root[i]["pose"][11].asFloat();
                if (curr == target) {
                    INFO("Viewpoint " << target << " must be self-reachable");
                    // Every viewpoint must be self reachable
                    REQUIRE(locs.find(target) != locs.end());
                    // We should never be at a not included viewpoint
                    CHECK(included[i]);
                    ViewpointPtr target_viewpoint = locs[target];
                    CHECK(target_viewpoint->point.x == Approx(tar_x));
                    CHECK(target_viewpoint->point.y == Approx(tar_y));
                    CHECK(target_viewpoint->point.z == Approx(tar_z));
                    navigableCount++;
                } else if (!currentViewpoint["unobstructed"][i].asBool()) {
                    // obstructed
                    INFO("Viewpoint " << target << " is obstructed from "
                          << curr << ", can't be a navigableLocation");
                    CHECK(locs.find(target) == locs.end());
                } else if (!included[i]) {
                    INFO("Viewpoint " << target << " is excluded,"
                          << " can't be a navigableLocation");
                    CHECK(locs.find(target) == locs.end());
                } else {
                    // check if this viewpoint is visible
                    INFO("atan2 " << atan2(tar_y - y, tar_x - x));
                    float viewpointHeading = M_PI/2 - atan2(tar_y - y, tar_x - x);
                    // convert interval [-0.5pi, 1.5pi] to interval [0, 2pi]
                    if (viewpointHeading < 0) {
                        viewpointHeading += 2*M_PI;
                    }
                    bool visible = fabs(state->heading - viewpointHeading) <= half_hfov ||
                          fabs(state->heading + 2.0 * M_PI - viewpointHeading) <= half_hfov ||
                          fabs(state->heading - (viewpointHeading + 2.0 * M_PI)) <= half_hfov;
                    INFO("Estimated heading " << viewpointHeading << ", agent heading " << state->heading
                        << ", visible " << visible);
                    if (visible) {
                        INFO("Viewpoint " << target << " (" << tar_x << ", " << tar_y << ", " << tar_z 
                            << ") should be reachable from "  << curr << " (" << x << ", " << y << ", " << z 
                            << ") with heading " << state->heading);
                        REQUIRE(locs.find(target) != locs.end());
                        ViewpointPtr target_viewpoint = locs[target];
                        CHECK(target_viewpoint->point.x == Approx(tar_x));
                        CHECK(target_viewpoint->point.y == Approx(tar_y));
                        CHECK(target_viewpoint->point.z == Approx(tar_z));
                        navigableCount++;
                    } else {
                        INFO("Viewpoint " << target << " (" << tar_x << ", " << tar_y << ", " << tar_z 
                            << ") is not visible in camera from "  << curr << " (" << x << ", " << y << ", " << z 
                            << ") with heading " << state->heading << ", can't be a navigableLocation");
                        REQUIRE(locs.find(target) == locs.end());
                    }
                }
            }
            CHECK(navigableCount == state->navigableLocations.size());

            // Move somewhere else
            std::vector<ViewpointPtr> actions = state->navigableLocations;
            int ix = t % actions.size(); // select an action
            sim.makeAction(ix, radians(heading_chg[t]), radians(elevation_chg[t]));
        }
    }
    REQUIRE_NOTHROW(sim.close());
}


TEST_CASE( "Simulator state->rgb is correct", "[Simulator, Rendering]" ) {

  //TODO - switching lots of different episodes on the same simulator
}


