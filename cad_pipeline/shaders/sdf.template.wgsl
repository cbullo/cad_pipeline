//)"
@group(0)
@binding(0)
var<storage, write> v_internal_indices: array<vec4<f32>>;
var<storage, write> v_boundary_indices: array<vec4<f32>>;

fn sdf_func(vec3<f32> p, vec3<f32 d) {
  {}  
}

@compute
@workgroup_size(1)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
  // camera movement	
	float an = 0.0;
	vec3 ro = vec3( 1.0*cos(an), 0.4, 1.0*sin(an) );
  vec3 ta = vec3( 0.0, 0.0, 0.0 );
  // camera matrix
  vec3 ww = normalize( ta - ro );
  vec3 uu = normalize( cross(ww,vec3(0.0,1.0,0.0) ) );
  vec3 vv = normalize( cross(uu,ww));

  vec3 tot = vec3(0.0);
  
  vec2 p(global_id.x / resolution.x, global_id.y / resolution.y);

  // create view ray
  vec3 rd = normalize( p.x*uu + p.y*vv + 1.5*ww );

  // raymarch
  const float tmax = 3.0;
  float t = 0.0;
  for( int i=0; i<256; i++ )
  {
      vec3 pos = ro + t*rd;
      float h = map(pos);
      if( h<0.0001 || t>tmax ) break;
      t += h;
  }
      
  
  // // shading/lighting	
  // vec3 col = vec3(0.0);
  // if( t<tmax )
  // {
  //     vec3 pos = ro + t*rd;
  //     vec3 nor = calcNormal(pos);
  //     float dif = clamp( dot(nor,vec3(0.57703)), 0.0, 1.0 );
  //     float amb = 0.5 + 0.5*dot(nor,vec3(0.0,1.0,0.0));
  //     col = vec3(0.2,0.3,0.4)*amb + vec3(0.8,0.7,0.5)*dif;
  // }

  // gamma        
  col = sqrt( col );
  tot += col;

	fragColor = vec4( tot, 1.0 );
}