struct OutputBuffer {
  counter: atomic<u32>,
  instance_count: u32,
  first_vertex: u32,
  first_instance: u32,
  v_positions: array<vec4<f32>>,
};

@group(0)
@binding(0)
var<storage, read_write> internal_vertices: OutputBuffer;
@group(0)
@binding(1)
var<storage, read_write> boundary_vertices: OutputBuffer;

//SDF_FUNCTIONS

fn sdf_func(p: vec3<f32>) -> f32 {
//SDF_INVOCATIONS  
  return 0.0;
}

var<workgroup> local_index_internal : atomic<u32>;
var<workgroup> local_index_boundary : atomic<u32>;

@compute
@workgroup_size(64)
fn main(@builtin(local_invocation_id) lid : vec3<u32>,
        @builtin(global_invocation_id) global_id: vec3<u32>) {

  // Initialize shared counter once per group
  if (lid.x == 0u) {
    atomicStore(&local_index_internal, 0u);
  }
  workgroupBarrier();

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

  let thread_index_boundary = atomicAdd(&local_index_boundary, 3u);
  let thread_index_internal = atomicAdd(&local_index_internal, 3u);

  workgroupBarrier();
      // One thread per group reserves a block in the global buffer
    var base : u32 = 0u;
    if (lid.x == 0u) {
        let count = atomicLoad(&local_index_internal);
        base = atomicAdd(&internal_vertices.counter, count);
        // Share base with rest of workgroup
        atomicStore(&local_index_internal, base);
    }
  workgroupBarrier();
  // Read the base (now stored back into localCounter)
  let base_internal = atomicLoad(&local_index_internal);
   
  
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

  let global_index = base + lid.x;
	//return vec4<f32>( tot, 1.0 );
  internal_vertices.v_positions[global_index] = vec4<f32>( 1.0, 1.0, 1.0, 1.0 );
}