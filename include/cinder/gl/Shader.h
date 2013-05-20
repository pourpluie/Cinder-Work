#pragma once

#include "cinder/Camera.h"
#include "cinder/gl/Fog.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/Material.h"

namespace cinder { namespace gl {
	
typedef std::shared_ptr<class Shader> ShaderRef;
	
class Shader
{
public:
	enum
	{
		NONE, PHONG, BLINN, LAMBERT
	} typedef LightingModel;
	
	enum Precision
	{
		LOW, MEDIUM, HIGH
	};
	
	/////////////////////////////////////////////////////////////////
	
	class UniformOptions
	{
	public:
		UniformOptions();
		
		UniformOptions&		enableColor( bool enabled = true );
		bool				isColorEnabled() const;
		
		UniformOptions&		enableFog( bool enabled = true );
		bool				isFogEnabled() const;
		
		UniformOptions&		enableMaterial( bool enabled = true );
		bool				isMaterialEnabled() const;
		
		UniformOptions&		enableTexture( bool enabled = true );
		bool				isTextureEnabled() const;
		
		UniformOptions&		setLightingModel( LightingModel lm );
		LightingModel		getLightingModel() const;
		
		UniformOptions&		setNumLights( size_t numLights );
		size_t				getNumLights() const;
		
		UniformOptions&		setPrecision( Precision p );
		Precision			getPrecision() const;
		
		bool				operator==( const UniformOptions& rhs ) const;
		bool				operator!=( const UniformOptions& rhs ) const;
		bool				operator<( const UniformOptions& rhs ) const;
	protected:
		bool				mColorEnabled;
		bool				mFogEnabled;
		LightingModel		mLightingModel;
		bool				mMaterialEnabled;
		size_t				mNumLights;
		Precision			mPrecision;
		bool				mTextureEnabled;
	};
	
	/////////////////////////////////////////////////////////////////
	
	static ShaderRef		create( const UniformOptions& options );
	~Shader();
	
	void					bind();
	void					unbind();
	void					update();
	
	const ColorAf&			getColor() const;
	void					setColor( const ColorAf& color );
	
	const Vec3f&			getEyePoint() const;
	void					setEyePoint( const Vec3f& v );
	
	const Fog&				getFog() const;
	void					setFog( const Fog& fog );
	
	GlslProgRef				getGlslProg() const;
	
	const Light&			getLight( size_t id = 0 ) const;
	void					setLight( size_t id, const Light& light );
	
	const Material&			getMaterial() const;
	void					setMaterial( const Material& material );
	
	const Matrix44f&		getModelViewProjectionMatrix();
	void					setModelViewProjectionMatrix( const Matrix44f& m );
	
	void					setCamera( const Camera& c );
	
	GLuint					getTextureId() const;
	void					setTextureId( GLuint id );
	
	const UniformOptions&	getUniformOptions() const;
	
	const std::string&		getFragmentString() const;
	const std::string&		getVertexString() const;
protected:
	Shader( const UniformOptions& options );
	
	static const size_t		MAX_LIGHTS = 8;
	
	UniformOptions			mUniformOptions;
	
	std::string				mFragmentString;
	std::string				mVertexString;
	
	GlslProgRef				mGlslProg;
	GLuint					mHnd;
	
	ColorAf					mColor;
	Vec3f					mEyePoint;
	Fog						mFog;
	Material				mMaterial;
	Matrix44f				mModelViewProjectionMatrix;
	GLuint					mTextureId;
	
	std::vector<Light>		mLights;
	std::vector<float>		mLightAttenuationConstant;
	std::vector<float>		mLightAttenuationLinear;
	std::vector<float>		mLightAttenuationQuadratic;
	std::vector<ColorAf>	mLightColorAmbient;
	std::vector<ColorAf>	mLightColorDiffuse;
	std::vector<ColorAf>	mLightColorSpecular;
	std::vector<Vec3f>		mLightDirection;
	std::vector<Vec3f>		mLightPosition;
	std::vector<float>		mLightShine;
	std::vector<Light::LightType>	mLightType;

	GLuint					mUniformLocColor;
	GLuint					mUniformLocEyePoint;
	GLuint					mUniformLocModelViewProjectionMatrix;
	GLuint					mUniformLocTextureId;
	
	GLuint 					mUniformLocFogColor;
	GLuint 					mUniformLocFogDensity;
	GLuint 					mUniformLocFogEnd;
	GLuint 					mUniformLocFogScale;
	GLuint 					mUniformLocFogStart;
	
	GLuint 					mUniformLocLightAttenuationConstant[ MAX_LIGHTS ];
	GLuint 					mUniformLocLightAttenuationLinear[ MAX_LIGHTS ];
	GLuint 					mUniformLocLightAttenuationQuadratic[ MAX_LIGHTS ];
	GLuint					mUniformLocLightColorAmbient[ MAX_LIGHTS ];
	GLuint 					mUniformLocLightColorDiffuse[ MAX_LIGHTS ];
	GLuint 					mUniformLocLightColorSpecular[ MAX_LIGHTS ];
	GLuint 					mUniformLocLightDirection[ MAX_LIGHTS ];
	GLuint 					mUniformLocLightPosition[ MAX_LIGHTS ];
	GLuint 					mUniformLocLightShine[ MAX_LIGHTS ];
	GLuint					mUniformLocLightType[ MAX_LIGHTS ];
	
	GLuint 					mUniformLocMaterialAmbient;
	GLuint 					mUniformLocMaterialColor;
	GLuint 					mUniformLocMaterialDiffuse;
	GLuint 					mUniformLocMaterialEmissive;
	GLuint 					mUniformLocMaterialSpecular;
};
	
} }
