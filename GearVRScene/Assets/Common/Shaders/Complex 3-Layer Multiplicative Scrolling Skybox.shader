// - Unlit
// - Scroll 2 layers /w Multiplicative op

Shader "Custom/Complex 3-Layer Multiplicative Scrolling Skybox" {
Properties {
	_MainTex ("Base layer (RGB)", 2D) = "white" {}
	_DetailTex ("2nd layer (RGB)", 2D) = "white" {}
	_StarTex ("3rd layer (RGB)", 2D) = "white" {}
	_ScrollX ("Base layer Scroll speed X", Float) = 1.0
	_ScrollY ("Base layer Scroll speed Y", Float) = 0.0
	_Scroll2X ("2nd layer Scroll speed X", Float) = 1.0
	_Scroll2Y ("2nd layer Scroll speed Y", Float) = 0.0
	_Scroll3X ("3rd layer Scroll speed X", Float) = 1.0
	_Scroll3Y ("3rd layer Scroll speed Y", Float) = 0.0
	_AMultiplier ("Layer Multiplier", Float) = 0.5
}

SubShader {
	Tags { "Queue"="Geometry+10" "RenderType"="Opaque" }
	
	Lighting Off Fog { Mode Off }
	ZWrite Off
	
	LOD 100
	
		
	CGINCLUDE
	#pragma multi_compile LIGHTMAP_OFF LIGHTMAP_ON
	#include "UnityCG.cginc"
	sampler2D _MainTex;
	sampler2D _DetailTex;
	sampler2D _StarTex;
	
	float4 _MainTex_ST;
	float4 _DetailTex_ST;
	float4 _StarTex_ST;
	
	float _ScrollX;
	float _ScrollY;
	float _Scroll2X;
	float _Scroll2Y;
	float _Scroll3X;
	float _Scroll3Y;
	float _AMultiplier;
	
	struct v2f {
		float4 pos : SV_POSITION;
		float2 uv : TEXCOORD0;
		float2 uv2 : TEXCOORD1;
		fixed4 color : TEXCOORD2;
		float2 uv3 : TEXCOORD3;
	};

	
	v2f vert (appdata_full v)
	{
		v2f o;
		o.pos = mul(UNITY_MATRIX_MVP, v.vertex);
		o.uv = TRANSFORM_TEX(v.texcoord.xy,_MainTex) + frac(float2(_ScrollX, _ScrollY) * _Time);
		o.uv2 = TRANSFORM_TEX(v.texcoord.xy,_DetailTex) + frac(float2(_Scroll2X, _Scroll2Y) * _Time);
		o.uv3 = TRANSFORM_TEX(v.texcoord.xy,_StarTex) + frac(float2(_Scroll3X, _Scroll3Y) * _Time);
		o.color = fixed4(_AMultiplier, _AMultiplier, _AMultiplier, _AMultiplier);

		return o;
	}
	ENDCG


	Pass {
		CGPROGRAM
		#pragma vertex vert
		#pragma fragment frag
		#pragma fragmentoption ARB_precision_hint_fastest		
		fixed4 frag (v2f i) : COLOR
		{
			fixed4 o;
			fixed4 tex = tex2D (_MainTex, i.uv);
			fixed4 tex2 = tex2D (_DetailTex, i.uv2);
			fixed4 tex3 = tex2D ( _StarTex, i.uv3);
			o = (( tex * tex2) * i.color ) + tex3;
			return o;
		}
		ENDCG 
	}	
}
}
