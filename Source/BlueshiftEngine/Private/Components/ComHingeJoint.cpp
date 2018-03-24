// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Render/Render.h"
#include "Components/ComTransform.h"
#include "Components/ComRigidBody.h"
#include "Components/ComHingeJoint.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Hinge Joint", ComHingeJoint, ComJoint)
BEGIN_EVENTS(ComHingeJoint)
END_EVENTS

void ComHingeJoint::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("anchor", "Anchor", Vec3, GetLocalAnchor, SetLocalAnchor, Vec3::zero, "Joint position in local space", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("angles", "Angles", Angles, GetLocalAngles, SetLocalAngles, Vec3::zero, "Joint angles in local space", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("useLimits", "Use Limits", bool, GetEnableLimitAngles, SetEnableLimitAngles, false, "Activate joint limits", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("minAngle", "Minimum Angle", float, GetMinimumAngle, SetMinimumAngle, 0.f, "Minimum value of joint angle", PropertyInfo::EditorFlag)
        .SetRange(-180, 0, 1);
    REGISTER_ACCESSOR_PROPERTY("maxAngle", "Maximum Angle", float, GetMaximumAngle, SetMaximumAngle, 0.f, "Maximum value of joint angle", PropertyInfo::EditorFlag)
        .SetRange(0, 180, 1);
    REGISTER_ACCESSOR_PROPERTY("motorTargetVelocity", "Motor Target Velocity", float, GetMotorTargetVelocity, SetMotorTargetVelocity, 0.f, "Target angular velocity (degree/s) of motor", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("maxMotorImpulse", "Maximum Motor Impulse", float, GetMaxMotorImpulse, SetMaxMotorImpulse, 0.f, "Maximum motor impulse", PropertyInfo::EditorFlag)
        .SetRange(0, 1e30f, 0.03f);
}

ComHingeJoint::ComHingeJoint() {
}

ComHingeJoint::~ComHingeJoint() {
}

void ComHingeJoint::Init() {
    ComJoint::Init();

    // Mark as initialized
    SetInitialized(true);
}

void ComHingeJoint::Start() {
    ComJoint::Start();

    const ComTransform *transform = GetEntity()->GetTransform();
    const ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();
    assert(rigidBody);

    PhysConstraintDesc desc;
    desc.type = PhysConstraint::Hinge;
    desc.collision = collisionEnabled;
    desc.breakImpulse = breakImpulse;

    desc.bodyA = rigidBody->GetBody();
    desc.axisInA = localAxis;
    desc.anchorInA = transform->GetScale() * localAnchor;

    if (connectedBody) {
        Mat3 worldAxis = desc.bodyA->GetAxis() * localAxis;
        Vec3 worldAnchor = desc.bodyA->GetOrigin() + desc.bodyA->GetAxis() * desc.anchorInA;

        desc.bodyB = connectedBody->GetBody();
        desc.axisInB = connectedBody->GetBody()->GetAxis().TransposedMul(worldAxis);
        desc.anchorInB = connectedBody->GetBody()->GetAxis().TransposedMulVec(worldAnchor - connectedBody->GetBody()->GetOrigin());

        connectedAxis = desc.axisInB;
        connectedAnchor = desc.anchorInB;
    } else {
        desc.bodyB = nullptr;

        connectedAxis = Mat3::identity;
        connectedAnchor = Vec3::origin;
    }

    constraint = physicsSystem.CreateConstraint(&desc);

    PhysHingeConstraint *hingeConstraint = static_cast<PhysHingeConstraint *>(constraint);

    // Apply limit angles
    hingeConstraint->SetLimitAngles(DEG2RAD(minimumAngle), DEG2RAD(maximumAngle));
    hingeConstraint->EnableLimitAngles(enableLimitAngles);
    
    // Apply motor
    if (motorTargetVelocity != 0.0f) {
        hingeConstraint->SetMotor(DEG2RAD(motorTargetVelocity), maxMotorImpulse);
        hingeConstraint->EnableMotor(true);
    }

    if (IsActiveInHierarchy()) {
        constraint->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

const Vec3 &ComHingeJoint::GetLocalAnchor() const {
    return localAnchor;
}

void ComHingeJoint::SetLocalAnchor(const Vec3 &anchor) {
    this->localAnchor = anchor;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetFrameA(anchor, localAxis);
    }
}

Angles ComHingeJoint::GetLocalAngles() const {
    return localAxis.ToAngles();
}

void ComHingeJoint::SetLocalAngles(const Angles &angles) {
    this->localAxis = angles.ToMat3();
    this->localAxis.FixDegeneracies();

    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetFrameA(localAnchor, localAxis);
    }
}

const Vec3 &ComHingeJoint::GetConnectedAnchor() const {
    return connectedAnchor;
}

void ComHingeJoint::SetConnectedAnchor(const Vec3 &anchor) {
    this->connectedAnchor = anchor;
    if (constraint) {
        ((PhysP2PConstraint *)constraint)->SetAnchorB(anchor);
    }
}

Angles ComHingeJoint::GetConnectedAngles() const {
    return connectedAxis.ToAngles();
}

void ComHingeJoint::SetConnectedAngles(const Angles &angles) {
    this->connectedAxis = angles.ToMat3();
    this->connectedAxis.FixDegeneracies();

    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetFrameB(connectedAnchor, connectedAxis);
    }
}

bool ComHingeJoint::GetEnableLimitAngles() const {
    return enableLimitAngles;
}

void ComHingeJoint::SetEnableLimitAngles(bool enable) {
    this->enableLimitAngles = enable;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->EnableLimitAngles(enableLimitAngles);
    }
}

float ComHingeJoint::GetMinimumAngle() const {
    return minimumAngle;
}

void ComHingeJoint::SetMinimumAngle(float minimumAngle) {
    this->minimumAngle = minimumAngle;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetLimitAngles(DEG2RAD(minimumAngle), DEG2RAD(maximumAngle));
    }
}

float ComHingeJoint::GetMaximumAngle() const {
    return maximumAngle;
}

void ComHingeJoint::SetMaximumAngle(float maximumAngle) {
    this->maximumAngle = maximumAngle;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetLimitAngles(DEG2RAD(minimumAngle), DEG2RAD(maximumAngle));
    }
}

float ComHingeJoint::GetMotorTargetVelocity() const {
    return motorTargetVelocity;
}

void ComHingeJoint::SetMotorTargetVelocity(float motorTargetVelocity) {
    this->motorTargetVelocity = motorTargetVelocity;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetMotor(DEG2RAD(motorTargetVelocity), maxMotorImpulse);
        ((PhysHingeConstraint *)constraint)->EnableMotor(motorTargetVelocity != 0.0f ? true : false);
    }
}

float ComHingeJoint::GetMaxMotorImpulse() const {
    return maxMotorImpulse;
}

void ComHingeJoint::SetMaxMotorImpulse(float maxMotorImpulse) {
    this->maxMotorImpulse = maxMotorImpulse;
    if (constraint) {
        ((PhysHingeConstraint *)constraint)->SetMotor(DEG2RAD(motorTargetVelocity), maxMotorImpulse);
        ((PhysHingeConstraint *)constraint)->EnableMotor(motorTargetVelocity != 0.0f ? true : false);
    }
}

void ComHingeJoint::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    const ComTransform *transform = GetEntity()->GetTransform();

    if (transform->GetOrigin().DistanceSqr(sceneView.origin) < 20000.0f * 20000.0f) {
        Vec3 worldOrigin = transform->GetTransform() * localAnchor;
        Mat3 worldAxis = transform->GetAxis() * localAxis;

        Mat3 constraintAxis = Mat3::identity;
        if (connectedBody) {
            constraintAxis = connectedBody->GetEntity()->GetTransform()->GetAxis();
        }

        if (enableLimitAngles) {
            renderWorld->SetDebugColor(Color4::yellow, Color4::yellow * 0.5f);
            renderWorld->DebugArc(worldOrigin, constraintAxis[0], constraintAxis[1], CentiToUnit(2.5), minimumAngle, maximumAngle, true);

            renderWorld->SetDebugColor(Color4::red, Color4::zero);
            renderWorld->DebugLine(worldOrigin, worldOrigin + worldAxis[0] * CentiToUnit(2.5), 1);
        }

        renderWorld->SetDebugColor(Color4::red, Color4::red);
        renderWorld->DebugArrow(worldOrigin - worldAxis[2] * CentiToUnit(5), worldOrigin + worldAxis[2] * CentiToUnit(5), CentiToUnit(3), CentiToUnit(0.75));
    }
}

BE_NAMESPACE_END
