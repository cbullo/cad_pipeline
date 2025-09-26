@group(0)
@binding(0)
var<storage, read_write> v_internal_indices: array<vec4<f32>>;
@group(0)
@binding(1)
var<storage, read_write> v_boundary_indices: array<vec4<f32>>;
@group(0)
@binding(2)
var<storage, read_write> indirect_draw: array<f32>;


//SDF_FUNCTIONS

fn sdf_func(p: vec3<f32>) -> f32 {
//SDF_INVOCATIONS  
  return 0.0;
}

struct Uniforms {
    resolution: vec2<f32>
}

//@group(0) @binding(2)
//var<uniform> uniforms: Uniforms;

@compute
@workgroup_size(1)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) -> @location(0) vec4<f32> {
  // camera movement	
	let an: f32 = 0.0;
	let ro: vec3<f32> = vec3<f32>( 1.0*cos(an), 0.4, 1.0*sin(an) );
  let ta: vec3<f32> = vec3<f32>( 0.0, 0.0, 0.0 );
  // camera matrix
  let ww: vec3<f32> = normalize( ta - ro );
  let uu: vec3<f32> = normalize( cross(ww,vec3<f32>(0.0,1.0,0.0) ) );
  let vv: vec3<f32> = normalize( cross(uu,ww));

  var tot: vec3<f32> = vec3<f32>(0.0);
  
  let p = vec2<f32>(f32(global_id.x) / 1024.0, f32(global_id.y) / 1024.0);

  // create view ray
  let rd = normalize( p.x*uu + p.y*vv + 1.5*ww );

  // raymarch
  const tmax: f32 = 3.0;
  var t: f32 = 0.0;
  for (var i = 0; i < 256; i=i+1)
  {
      let pos = ro + t*rd;
      let h = sdf_func(pos);
      if (h<0.0001 || t>tmax) { 
        break; 
      }
      t += h;
  }
      
  
  // // shading/lighting	
  var col: vec3<f32> = vec3<f32>(0.0);
  // if( t<tmax )
  // {
  //     vec3<f32> pos = ro + t*rd;
  //     vec3<f32> nor = calcNormal(pos);
  //     f32 dif = clamp( dot(nor,vec3<f32>(0.57703)), 0.0, 1.0 );
  //     f32 amb = 0.5 + 0.5*dot(nor,vec3<f32>(0.0,1.0,0.0));
  //     col = vec3<f32>(0.2,0.3,0.4)*amb + vec3<f32>(0.8,0.7,0.5)*dif;
  // }

  // gamma        
  col = sqrt( col );
  tot += col;

	return vec4<f32>( tot, 1.0 );
}