Shader "Custom/CheapGlass" {
     
    Properties {
       _EnvMap ("EnvMap", 2D) = "black" { TexGen SphereMap }
       }
     
    SubShader {
       SeparateSpecular On
       Tags {"Queue" = "Transparent" }
     
          Pass {
             Name "BASE"
             ZWrite on
             //Blend One One                       // additive
             Blend One OneMinusSrcColor          // soft additive
             //Blend SrcAlpha OneMinusSrcAlpha     // real alpha blending
             BindChannels {
             Bind "Vertex", vertex
             Bind "normal", normal
          }
     
          SetTexture [_EnvMap] {
             combine texture
          }
       }
    }
     
    Fallback off
    }

 