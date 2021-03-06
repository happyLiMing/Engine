#include "Engine/UI/WidgetBase.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Fonts/BitmapFont.hpp"
#include "Engine/Math/Matrix4x4.hpp"
#include "Engine/Core/Events/EventSystem.hpp"
#include "UISystem.hpp"

//-----------------------------------------------------------------------------------
WidgetBase::WidgetBase()
    : m_name("Unnamed Widget")
{
    m_propertiesForAllStates.Set<std::string>("Name", m_name);
    m_propertiesForAllStates.Set<Vector2>("Offset", Vector2::ZERO);
    m_propertiesForAllStates.Set<Vector2>("Size", Vector2::ONE);
    m_propertiesForAllStates.Set<Vector2>("Padding", Vector2::ZERO);
    m_propertiesForAllStates.Set<RGBA>("BackgroundColor", RGBA::LIGHT_GRAY);
    m_propertiesForAllStates.Set<RGBA>("BorderColor", RGBA::GRAY);
    m_propertiesForAllStates.Set<float>("Opacity", 1.0f);
    m_propertiesForAllStates.Set<float>("BorderWidth", 5.0f);
    SetProperty("BorderColor", RGBA::BLACK, DISABLED_WIDGET_STATE);
    SetProperty("TextColor", RGBA::GRAY, DISABLED_WIDGET_STATE);
    SetProperty("BackgroundColor", RGBA::DARK_GRAY, DISABLED_WIDGET_STATE);
}

//-----------------------------------------------------------------------------------
WidgetBase::~WidgetBase()
{
    for (WidgetBase* child : m_children)
    {
        delete child;
    }
}

//-----------------------------------------------------------------------------------
void WidgetBase::Update(float deltaSeconds)
{
    if (IsHidden())
    {
        return;
    }
    UpdateChildren(deltaSeconds);
}

//-----------------------------------------------------------------------------------
void WidgetBase::UpdateChildren(float deltaSeconds)
{
    for (WidgetBase* child : m_children)
    {
        child->Update(deltaSeconds);
    }
}

//-----------------------------------------------------------------------------------
void WidgetBase::Render() const
{
    if (IsHidden())
    {
        return;
    }
    float borderWidth = GetProperty<float>("BorderWidth");
    RGBA bgColor = GetProperty<RGBA>("BackgroundColor");
    RGBA borderColor = GetProperty<RGBA>("BorderColor");
    float opacity = GetProperty<float>("Opacity");

    opacity *= GetParentOpacities();
    bgColor.alpha = (uchar)((((float)bgColor.alpha / 255.0f) * opacity) * 255.0f);
    borderColor.alpha = (uchar)((((float)borderColor.alpha / 255.0f) * opacity) * 255.0f);

    if (borderWidth > 0.0f)
    {
        Renderer::instance->DrawAABB(m_borderedBounds, borderColor);
    }
    if (bgColor.alpha > 0.0f)
    {
        Renderer::instance->DrawAABB(m_borderlessBounds, bgColor);
    }
}

//-----------------------------------------------------------------------------------
void WidgetBase::RenderChildren() const
{
    for (WidgetBase* child : m_children)
    {
        child->Render();
    }
}

//-----------------------------------------------------------------------------------
void WidgetBase::AddChild(WidgetBase* child)
{
    m_children.push_back(child);
    child->m_parent = this;
    child->RecalculateBounds();
    RecalculateBounds();
}

//-----------------------------------------------------------------------------------
AABB2 WidgetBase::GetSmallestBoundsAroundChildren()
{
    AABB2 smallestBounds = m_children.size() > 0 ? m_children.at(0)->m_bounds : AABB2(Vector2::ZERO, Vector2::ZERO);

    for (WidgetBase* child : m_children)
    {
        AABB2 childBounds = child->GetBounds();
        smallestBounds.mins.x = (childBounds.mins.x < smallestBounds.mins.x) ? childBounds.mins.x : smallestBounds.mins.x;
        smallestBounds.mins.y = (childBounds.mins.y < smallestBounds.mins.y) ? childBounds.mins.y : smallestBounds.mins.y;
        smallestBounds.maxs.x = (childBounds.maxs.x > smallestBounds.maxs.x) ? childBounds.maxs.x : smallestBounds.maxs.x;
        smallestBounds.maxs.y = (childBounds.maxs.y > smallestBounds.maxs.y) ? childBounds.maxs.y : smallestBounds.maxs.y;
    }

    return smallestBounds;
}

//-----------------------------------------------------------------------------------
void WidgetBase::BuildFromXMLNode(XMLNode& node)
{
    const char* nameAttribute = node.getAttribute("Name");
    const char* horizontalOffset = node.getAttribute("HorizontalOffset");
    const char* verticalOffset = node.getAttribute("VerticalOffset");
    const char* backgroundColorAttribute = node.getAttribute("BackgroundColor");
    const char* borderColorAttribute = node.getAttribute("BorderColor");
    const char* borderWidthAttribute = node.getAttribute("BorderWidth");
    const char* onClickAttribute = node.getAttribute("OnClick");
    const char* opacityAttribute = node.getAttribute("Opacity");
    const char* sizeAttribute = node.getAttribute("Size");
    const char* offsetAttribute = node.getAttribute("Offset");
    const char* paddingAttribute = node.getAttribute("Padding");

    Vector2 offset = m_propertiesForAllStates.Get<Vector2>("Offset");
    RGBA bgColor = m_propertiesForAllStates.Get<RGBA>("BackgroundColor");
    RGBA edgeColor = m_propertiesForAllStates.Get<RGBA>("BorderColor");

    if (paddingAttribute)
    {
        Vector2 padding = Vector2::CreateFromString(paddingAttribute);
        m_propertiesForAllStates.Set<Vector2>("Padding", padding);
    }
    if (offsetAttribute)
    {
        offset = Vector2::CreateFromString(offsetAttribute);
    }
    if (horizontalOffset)
    {
        offset.x = std::stof(horizontalOffset);
    }
    if (verticalOffset)
    {
        offset.y = std::stof(verticalOffset);
    }
    if (backgroundColorAttribute)
    {
        bgColor = RGBA::CreateFromString(backgroundColorAttribute);
        m_propertiesForAllStates.Set<RGBA>("BackgroundColor", bgColor);
    }
    if (borderColorAttribute)
    {
        edgeColor = RGBA::CreateFromString(borderColorAttribute);
        m_propertiesForAllStates.Set<RGBA>("BorderColor", edgeColor);
    }
    if (borderWidthAttribute)
    {
        m_propertiesForAllStates.Set<float>("BorderWidth", std::stof(borderWidthAttribute));
    }
    if (onClickAttribute)
    {
        std::string eventName = std::string(onClickAttribute);
        m_propertiesForAllStates.Set("OnClick", eventName);
    }
    if (nameAttribute)
    {
        std::string name = std::string(nameAttribute);
        m_propertiesForAllStates.Set("Name", name);
        m_name = name;
    }
    if (opacityAttribute)
    {
        m_propertiesForAllStates.Set<float>("Opacity", std::stof(opacityAttribute));
    }
    if (sizeAttribute)
    {
        Vector2 size = Vector2::CreateFromString(sizeAttribute);
        m_propertiesForAllStates.Set<Vector2>("Size", size);
    }

    m_propertiesForAllStates.Set<Vector2>("Offset", offset);
    m_propertiesForAllStates.Set<Vector2>("Size", Vector2::ONE);

    std::vector<XMLNode> children = XMLUtils::GetChildren(node);
    for (XMLNode& child : children)
    {
        if (!child.isEmpty())
        {
            AddChild(UISystem::instance->CreateWidget(child));
        }
    }
}

//-----------------------------------------------------------------------------------
void WidgetBase::OnClick()
{
    std::string clickEvent;
    PropertyGetResult state = m_propertiesForAllStates.Get("OnClick", clickEvent);
    if (state == PGR_SUCCESS)
    {
        EventSystem::FireEvent(clickEvent);
    }
    UnsetPressed();
}

//-----------------------------------------------------------------------------------
Vector2 WidgetBase::GetParentOffsets() const
{
    Vector2 parentOffsets = Vector2::ZERO;
    WidgetBase* parent = m_parent;
    while (parent)
    {
        parentOffsets += parent->m_propertiesForAllStates.Get<Vector2>("Offset");
        parent = parent->m_parent;
    }
    return parentOffsets;
}

//-----------------------------------------------------------------------------------
float WidgetBase::GetParentOpacities() const
{
    float parentOpacities = 1.0f;
    WidgetBase* parent = m_parent;
    while (parent)
    {
        parentOpacities *= parent->m_propertiesForAllStates.Get<float>("Opacity");
        parent = parent->m_parent;
    }
    return parentOpacities;
}

//-----------------------------------------------------------------------------------
Matrix4x4 WidgetBase::GetModelMatrix() const
{
    Matrix4x4 model = Matrix4x4::IDENTITY;
    Matrix4x4::MatrixMakeTranslation(&model, Vector3(m_propertiesForAllStates.Get<Vector2>("Offset"), 0.0f));
    return model;
}

//-----------------------------------------------------------------------------------
bool WidgetBase::SetWidgetVisibility(const std::string& name, bool setHidden)
{
    for (WidgetBase* child : m_children)
    {
        if (child->GetProperty<std::string>("Name") == name)
        {
            setHidden ? child->SetHidden() : child->SetVisible();
            return true;
        }
        else if (child->SetWidgetVisibility(name, setHidden))
        {
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------------
void WidgetBase::SetHidden()
{
    m_currentState = HIDDEN_WIDGET_STATE;
    for (WidgetBase* child : m_children)
    {
        child->SetHidden();
    }
}

//-----------------------------------------------------------------------------------
void WidgetBase::SetVisible()
{
    m_currentState = ACTIVE_WIDGET_STATE;
    for (WidgetBase* child : m_children)
    {
        child->SetVisible();
    }
}

//-----------------------------------------------------------------------------------
bool WidgetBase::IsClickable()
{
    return !(m_currentState == DISABLED_WIDGET_STATE || m_currentState == HIDDEN_WIDGET_STATE);
}

//-----------------------------------------------------------------------------------
WidgetBase* WidgetBase::GetWidgetPointIsInside(const Vector2& point)
{
    for (WidgetBase* child : m_children) //Ask in the appropriate drawing order
    {
        WidgetBase* widget = child->GetWidgetPointIsInside(point);
        if (widget)
        {
            return widget;
        }
    }
    return m_bounds.IsPointOnOrInside(point) ? this : nullptr;
}