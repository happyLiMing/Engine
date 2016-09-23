#include "Engine/Renderer/AABB3.hpp"

//-----------------------------------------------------------------------------------
AABB3::AABB3()
{
}

//-----------------------------------------------------------------------------------
AABB3::AABB3(const Vector3& Mins, const Vector3& Maxs) : mins(Mins), maxs(Maxs)
{

}

//-----------------------------------------------------------------------------------
AABB3::~AABB3()
{
}

//-----------------------------------------------------------------------------------
AABB3& AABB3::operator-=(const Vector3& rhs)
{
	this->mins -= rhs;
	this->maxs -= rhs;
	return *this;
}

//-----------------------------------------------------------------------------------
AABB3& AABB3::operator+=(const Vector3& rhs)
{
	this->mins += rhs;
	this->maxs += rhs;
	return *this;
}
