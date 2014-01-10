#pragma once

#include "Quake.h"
#include "cinder/Vector.h"
#include "cinder/gl/Texture.h"
#include <list>
#include <string>

class Earth {
 public:
	Earth();
	Earth( const ci::gl::TextureRef &aTexDiffuse, const ci::gl::TextureRef &aTexNormal, const ci::gl::TextureRef &aTexMask );
	
	void setQuakeLocTip();
	void update();
	void repelLocTips();
	void draw();
	void drawQuakes();
	void drawQuakeLabelsOnBillboard( const ci::Vec3f &aRight, const ci::Vec3f &aUp );
	void drawQuakeLabelsOnSphere( const ci::Vec3f aEyeNormal, const float aEyeDist );
	void drawQuakeVectors();
	void addQuake( float aLat, float aLong, float aMag, std::string aTitle );
	void setMinMagToRender( float amt );
	ci::Vec3f mLoc;
	float mRadius;
	ci::gl::TextureRef mTexDiffuse;
	ci::gl::TextureRef mTexNormal;
	ci::gl::TextureRef mTexMask;
	std::list<Quake> mQuakes;
	float mMinMagToRender;
};