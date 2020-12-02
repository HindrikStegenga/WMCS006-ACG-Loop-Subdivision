#version 410
// Fragment shader

layout (location = 0) in vec3 vertcoords_camera_fs;
layout (location = 1) in vec3 vertnormal_camera_fs;
layout (location = 2) in vec3 vertnormal_world_fs;

uniform bool drawReflectionLines;
uniform float sineScale;
uniform vec3 testNormal;
uniform bool selectionMode;
uniform bool selectLine;

out vec4 fColor;

void main() {

   // lineSelection gives orange fragments back
   if(selectLine) {
       fColor = vec4(1, 0.27, 0, 1);
       return;
   }

  // selectionMode gives red fragments back.
  if (selectionMode) {
    fColor = vec4(1, 0, 0, 1);
    return;
  }


  vec3 lightpos = vec3(3.0, 0.0, 2.0)*10.0;
  vec3 lightcolour = vec3(1.0);

  vec3 matcolour = vec3(0.53, 0.80, 0.87);
  vec3 matspeccolour = vec3(1.0);



  if (drawReflectionLines) {
      float dotProduct = dot(normalize(vertnormal_world_fs), testNormal);
      float sine = sin(dotProduct * sineScale);
      if (sine < 0.5) {
          matcolour = vec3(1);
      } else {
          matcolour = vec3(0);
      }

  }

  float matambientcoeff = 0.2;
  float matdiffusecoeff = 0.3;
  float matspecularcoeff = 0.8;

  vec3 normal;
  normal = normalize(vertnormal_camera_fs);
  
  vec3 surftolight = normalize(lightpos - vertcoords_camera_fs);
  float diffusecoeff = max(0.0, dot(surftolight, normal));
  
  vec3 camerapos = vec3(0.0);
  vec3 surftocamera = normalize(camerapos - vertcoords_camera_fs);
  vec3 reflected = 2.0 * diffusecoeff * normal - surftolight;
  float specularcoeff = max(0.0, dot(reflected, surftocamera));

  vec3 compcolour = min(1.0, matambientcoeff + matdiffusecoeff * diffusecoeff) * lightcolour * matcolour;
       compcolour += matspecularcoeff * specularcoeff * lightcolour * matspeccolour;


  fColor = vec4(compcolour, 1.0);



}
