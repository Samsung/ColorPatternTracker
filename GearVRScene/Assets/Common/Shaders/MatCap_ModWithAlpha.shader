Shader "MatCap/JDCustom"
{
	Properties
	{
		_MainTex ("Base (RGB) Transparency (A)", 2D) = "white" {}
		_MatCap ("MatCap (RGB)", 2D) = "white" {}
	}
	
Category {
	//ZWrite On
    //Alphatest Greater 0

	Subshader
	{	
		Pass
		{
			Tags { "Queue"="Transparent" "IgnoreProjector"="True" "RenderType"="Transparent" "LightMode" = "Always" }
			Blend SrcAlpha OneMinusSrcAlpha

			CGPROGRAM
				#pragma vertex vert
				#pragma fragment frag
				#pragma fragmentoption ARB_precision_hint_fastest
				#include "UnityCG.cginc"
				
				struct v2f
				{
					float4 pos	: SV_POSITION;
					float2 uv 	: TEXCOORD0;
					float2 cap	: TEXCOORD1;
				};
				
				uniform float4 _MainTex_ST;
				
				v2f vert (appdata_base v)
				{
					v2f o;
					o.pos = mul (UNITY_MATRIX_MVP, v.vertex);
					o.uv = TRANSFORM_TEX(v.texcoord, _MainTex);
					
					half2 capCoord;
					capCoord.x = dot(UNITY_MATRIX_IT_MV[0].xyz,v.normal);
					capCoord.y = dot(UNITY_MATRIX_IT_MV[1].xyz,v.normal);
					o.cap = capCoord * 0.5 + 0.5;
					
					return o;
				}
				
				uniform sampler2D _MainTex;
				uniform sampler2D _MatCap;
				
				fixed4 frag (v2f i) : COLOR
				{
					fixed4 tex = tex2D(_MainTex, i.uv);
					fixed4 mc = tex2D(_MatCap, i.cap);
					
					return (tex + (mc*2.0)-1.0);
				}
			ENDCG
		}
	}
}	
	Fallback "VertexLit"
}