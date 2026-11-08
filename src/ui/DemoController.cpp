#include "DemoController.h"
#include "UITheme.h"
#include <Arduino.h>

bool DemoController::isNavStep(Step step) {
    switch (step) {
        case Step::STRAIGHT:
        case Step::LEFT:
        case Step::RIGHT:
        case Step::MERGE_LEFT:
        case Step::MERGE_RIGHT:
        case Step::ROUNDABOUT:
        case Step::ROUNDABOUT_SHARP_LEFT_CW:
        case Step::ROUNDABOUT_SHARP_LEFT_CCW:
        case Step::ROUNDABOUT_SHARP_RIGHT_CW:
        case Step::ROUNDABOUT_SHARP_RIGHT_CCW:
        case Step::ROUNDABOUT_SLIGHT_LEFT_CW:
        case Step::ROUNDABOUT_SLIGHT_LEFT_CCW:
        case Step::ROUNDABOUT_SLIGHT_RIGHT_CW:
        case Step::ROUNDABOUT_SLIGHT_RIGHT_CCW:
        case Step::ROUNDABOUT_EXIT_CW:
        case Step::ROUNDABOUT_EXIT_CCW:
        case Step::ROUNDABOUT_UTURN_CW:
        case Step::ROUNDABOUT_UTURN_CCW:
        case Step::ROUNDABOUT_LEFT_CW:
        case Step::ROUNDABOUT_LEFT_CCW:
        case Step::ROUNDABOUT_RIGHT_CW:
        case Step::ROUNDABOUT_RIGHT_CCW:
        case Step::ROUNDABOUT_STRAIGHT_CW:
        case Step::ROUNDABOUT_STRAIGHT_CCW:
        case Step::UTURN:
        case Step::APPROACHING:
            return true;
        case Step::DEPART:
        case Step::ARRIVED:
        case Step::UNKNOWN_STATE:
            return false;
        default:
            return false;
    }
}

void DemoController::applyStep(UIState& state, Step step) {
    switch (step) {
        case Step::WAITING:
            state.screen = UIScreen::WAITING_FOR_PHONE;
            state.connected = false;
            break;

        case Step::IDLE:
            state.screen = UIScreen::IDLE;
            state.connected = true;
            state.clockHour = 9;
            state.clockMinute = 41;
            state.clockValid = true;
            break;

        case Step::DEPART:
            state.screen = UIScreen::NAVIGATION;
            state.connected = true;
            state.nav.maneuver = ManeuverType::DEPART;
            state.nav.roundaboutExitNumber = 0;
            break;

        case Step::STRAIGHT:
            state.screen = UIScreen::NAVIGATION;
            state.connected = true;
            state.nav.maneuver = ManeuverType::STRAIGHT;
            state.nav.distanceMeters = 800;
            state.nav.setStreetName("Main Street");
            state.nav.setEta("12:45");
            break;

        case Step::LEFT:
            state.nav.maneuver = ManeuverType::TURN_LEFT;
            state.nav.distanceMeters = 300;
            state.nav.setStreetName("Oak Avenue");
            state.nav.setEta("12:47");
            break;

        case Step::RIGHT:
            state.nav.maneuver = ManeuverType::TURN_RIGHT;
            state.nav.distanceMeters = 450;
            state.nav.setStreetName("Pine Road");
            state.nav.setEta("12:50");
            break;

        case Step::MERGE_LEFT:
            state.nav.maneuver = ManeuverType::MERGE_LEFT;
            state.nav.distanceMeters = 500;
            state.nav.setStreetName("I-95 North");
            state.nav.setEta("12:51");
            break;

        case Step::MERGE_RIGHT:
            state.nav.maneuver = ManeuverType::MERGE_RIGHT;
            state.nav.distanceMeters = 500;
            state.nav.setStreetName("I-95 South");
            state.nav.setEta("12:51");
            break;

        case Step::ROUNDABOUT:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_RIGHT;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            state.nav.roundaboutExitNumber = 3;
            break;

        // PNG-backed roundabout family - same demo distance/street/eta
        // pattern as the vector ROUNDABOUT step above, just cycling the
        // maneuver type so each PNG gets a turn on screen.
        case Step::ROUNDABOUT_SHARP_LEFT_CW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_SHARP_LEFT_CW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;
        case Step::ROUNDABOUT_SHARP_LEFT_CCW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_SHARP_LEFT_CCW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;
        case Step::ROUNDABOUT_SHARP_RIGHT_CW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_SHARP_RIGHT_CW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;
        case Step::ROUNDABOUT_SHARP_RIGHT_CCW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_SHARP_RIGHT_CCW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;
        case Step::ROUNDABOUT_SLIGHT_LEFT_CW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_SLIGHT_LEFT_CW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;
        case Step::ROUNDABOUT_SLIGHT_LEFT_CCW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_SLIGHT_LEFT_CCW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;
        case Step::ROUNDABOUT_SLIGHT_RIGHT_CW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_SLIGHT_RIGHT_CW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;
        case Step::ROUNDABOUT_SLIGHT_RIGHT_CCW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_SLIGHT_RIGHT_CCW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;
        case Step::ROUNDABOUT_EXIT_CW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_EXIT_CW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;
        case Step::ROUNDABOUT_EXIT_CCW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_EXIT_CCW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;
        case Step::ROUNDABOUT_UTURN_CW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_UTURN_CW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;
        case Step::ROUNDABOUT_UTURN_CCW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_UTURN_CCW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;
        case Step::ROUNDABOUT_LEFT_CW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_LEFT_CW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;
        case Step::ROUNDABOUT_LEFT_CCW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_LEFT_CCW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;
        case Step::ROUNDABOUT_RIGHT_CW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_RIGHT_CW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;
        case Step::ROUNDABOUT_RIGHT_CCW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_RIGHT_CCW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;
        case Step::ROUNDABOUT_STRAIGHT_CW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_STRAIGHT_CW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;
        case Step::ROUNDABOUT_STRAIGHT_CCW:
            state.nav.maneuver = ManeuverType::ROUNDABOUT_STRAIGHT_CCW;
            state.nav.distanceMeters = 600;
            state.nav.setStreetName("Victory Circle");
            state.nav.setEta("12:53");
            break;

        case Step::UTURN:
            state.nav.maneuver = ManeuverType::U_TURN_LEFT;
            state.nav.distanceMeters = 200;
            state.nav.setStreetName("Elm Street");
            state.nav.setEta("12:55");
            state.nav.roundaboutExitNumber = 0;
            break;

        case Step::APPROACHING:
            state.screen = UIScreen::TURN_APPROACHING;
            state.nav.maneuver = ManeuverType::TURN_RIGHT;
            state.nav.distanceMeters = 60;
            state.nav.setStreetName("Pine Road");
            state.nav.setEta("12:50");
            break;

        case Step::ARRIVED:
            state.screen = UIScreen::NAVIGATION;
            state.nav.maneuver = ManeuverType::ARRIVED;
            break;

        case Step::UNKNOWN_STATE:
            state.screen = UIScreen::NAVIGATION;
            state.nav.maneuver = ManeuverType::UNKNOWN;
            break;

        case Step::CALL:
            state.screen = UIScreen::INCOMING_CALL;
            state.call.setCallerName("John Rider");
            break;

        case Step::MUSIC:
            state.screen = UIScreen::MUSIC;
            state.music.setTitle("Highway Star");
            state.music.setArtist("Deep Purple");
            state.music.isPlaying = true;
            break;

        case Step::COUNT:
            break;
    }

    // Steps whose case body above doesn't set state.screen itself (unlike
    // STRAIGHT/APPROACHING/ARRIVED/UNKNOWN_STATE, which do) fall back to
    // NAVIGATION here.
    switch (step) {
        case Step::LEFT:
        case Step::RIGHT:
        case Step::MERGE_LEFT:
        case Step::MERGE_RIGHT:
        case Step::ROUNDABOUT:
        case Step::ROUNDABOUT_SHARP_LEFT_CW:
        case Step::ROUNDABOUT_SHARP_LEFT_CCW:
        case Step::ROUNDABOUT_SHARP_RIGHT_CW:
        case Step::ROUNDABOUT_SHARP_RIGHT_CCW:
        case Step::ROUNDABOUT_SLIGHT_LEFT_CW:
        case Step::ROUNDABOUT_SLIGHT_LEFT_CCW:
        case Step::ROUNDABOUT_SLIGHT_RIGHT_CW:
        case Step::ROUNDABOUT_SLIGHT_RIGHT_CCW:
        case Step::ROUNDABOUT_EXIT_CW:
        case Step::ROUNDABOUT_EXIT_CCW:
        case Step::ROUNDABOUT_UTURN_CW:
        case Step::ROUNDABOUT_UTURN_CCW:
        case Step::ROUNDABOUT_LEFT_CW:
        case Step::ROUNDABOUT_LEFT_CCW:
        case Step::ROUNDABOUT_RIGHT_CW:
        case Step::ROUNDABOUT_RIGHT_CCW:
        case Step::ROUNDABOUT_STRAIGHT_CW:
        case Step::ROUNDABOUT_STRAIGHT_CCW:
        case Step::UTURN:
            state.screen = UIScreen::NAVIGATION;
            break;
        default:
            break;
    }
}

void DemoController::update(UIState& state) {
    const uint32_t now = millis();

    if (!started_) {
        started_ = true;
        stepStartMs_ = now;
        lastDistanceTickMs_ = now;
        applyStep(state, currentStep_);
        return;
    }

    if (now - stepStartMs_ >= UITheme::kDemoStepIntervalMs) {
        currentStep_ = static_cast<Step>((static_cast<uint8_t>(currentStep_) + 1) % static_cast<uint8_t>(Step::COUNT));
        stepStartMs_ = now;
        lastDistanceTickMs_ = now;
        applyStep(state, currentStep_);
        return;
    }

    if (isNavStep(currentStep_) && now - lastDistanceTickMs_ >= UITheme::kDemoDistanceTickMs) {
        lastDistanceTickMs_ = now;
        if (state.nav.distanceMeters > UITheme::kDemoMinDistanceM + UITheme::kDemoDistanceStepM) {
            state.nav.distanceMeters -= UITheme::kDemoDistanceStepM;
        } else {
            state.nav.distanceMeters = UITheme::kDemoMinDistanceM;
        }
    }
}
