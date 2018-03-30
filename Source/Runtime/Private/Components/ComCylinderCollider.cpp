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
#include "Physics/Collider.h"
#include "Components/ComTransform.h"
#include "Components/ComCylinderCollider.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Cylinder Collider", ComCylinderCollider, ComCollider)
BEGIN_EVENTS(ComCylinderCollider)
END_EVENTS

void ComCylinderCollider::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("center", "Center", Vec3, GetCenter, SetCenter, Vec3::zero, 
        "", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("radius", "Radius", float, GetRadius, SetRadius, 50.0f, 
        "", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("height", "Height", float, GetHeight, SetHeight, 100.0f, 
        "", PropertyInfo::SystemUnits | PropertyInfo::EditorFlag);
}

ComCylinderCollider::ComCylinderCollider() {
}

ComCylinderCollider::~ComCylinderCollider() {
}

void ComCylinderCollider::CreateCollider() {
    if (collider) {
        colliderManager.ReleaseCollider(collider);
    }

    const ComTransform *transform = GetEntity()->GetTransform();

    Vec3 scaledCenter = transform->GetScale() * center;
    float scaledRadius = (transform->GetScale().ToVec2() * radius).MaxComponent();
    float scaledHeight = transform->GetScale().z * height;

    collider = colliderManager.AllocUnnamedCollider();
    collider->CreateCylinder(scaledCenter, scaledRadius, scaledHeight);
}

void ComCylinderCollider::SetCenter(const Vec3 &center) {
    this->center = center;

    if (IsInitialized()) {
        CreateCollider();
    }
}

void ComCylinderCollider::SetRadius(float radius) {
    this->radius = radius;

    if (IsInitialized()) {
        CreateCollider();
    }
}

void ComCylinderCollider::SetHeight(float height) {
    this->height = height;

    if (IsInitialized()) {
        CreateCollider();
    }
}

bool ComCylinderCollider::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    return false;
}

void ComCylinderCollider::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    if (selected) {
        const ComTransform *transform = GetEntity()->GetTransform();

        if (transform->GetOrigin().DistanceSqr(sceneView.origin) < 20000.0f * 20000.0f) {
            Vec3 scaledCenter = transform->GetScale() * center;
            float scaledRadius = (transform->GetScale().ToVec2() * radius).MaxComponent();
            float scaledHeight = transform->GetScale().z * height;

            Vec3 worldCenter = transform->GetTransform() * scaledCenter;

            renderWorld->SetDebugColor(Color4::orange, Color4::zero);
            renderWorld->DebugCylinderSimple(worldCenter, transform->GetAxis(), scaledHeight, scaledRadius + 0.25f, 1.25f, true);
        }
    }
}

BE_NAMESPACE_END