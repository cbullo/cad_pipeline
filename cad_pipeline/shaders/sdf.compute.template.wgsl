struct OutputBuffer {
  counter: atomic<u32>,
  instance_count: u32,
  first_index: u32,
  base_vertex: u32,
  first_instance: u32,
  v_indices: array<u32>,
};

struct Sth {
  v_vertices: array<vec4<f32>>,
};

struct Time {
  t: f32
};

@group(0)
@binding(0)
var<storage, read_write> internal_indices: OutputBuffer;

@group(0)
@binding(1)
var<storage, read_write> vertices: Sth;

@group(0)
@binding(2)
var<storage, read_write> time: Time;


//SDF_FUNCTIONS

fn sdf_func(p: vec3<f32>) -> f32 {
  var v = 0.0;
//SDF_INVOCATIONS  
  return v;
}

@compute
@workgroup_size(1)
fn reset_counter() {
  atomicStore(&internal_indices.counter, 0u);
  time.t += 0.01;
}

@compute
@workgroup_size(1)
fn output_vertices(@builtin(global_invocation_id) global_id: vec3<u32>) {
  let x = 2.0 * f32(global_id.x) / f32(1024/16) - 1.0;
  let y = 2.0 * f32(global_id.y) / f32(1024/16) - 1.0;

// camera movement	
	let an: f32 = time.t;
	let ro: vec3<f32> = vec3<f32>( 1.0*cos(an), 1.5 * sin(an), 1.0*sin(an) );
  let ta: vec3<f32> = vec3<f32>( 0.0, 0.0, 0.0 );
  // camera matrix
  let ww: vec3<f32> = normalize( ta - ro );
  let uu: vec3<f32> = normalize( cross(ww,vec3<f32>(0.0,1.0,0.0) ) );
  let vv: vec3<f32> = normalize( cross(uu,ww));

  var tot: vec3<f32> = vec3<f32>(0.0);
  
  let p = vec2<f32>(x, y );

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

  vertices.v_vertices[global_id.y * (1024/16 + 1) + global_id.x] = vec4(p.x, p.y, 0.0, 1.0);//vec4(ro + t * rd, 1.0); 
  if (t >= tmax) {
    vertices.v_vertices[global_id.y * (1024/16 + 1) + global_id.x].w = 0.0;
  }
}

var<workgroup> local_index_internal : atomic<u32>;

@compute
@workgroup_size(64)
fn output_indices(@builtin(local_invocation_id) lid : vec3<u32>,
        @builtin(global_invocation_id) global_id: vec3<u32>) {

  // Initialize shared counter once per group
  if (lid.x == 0u) {
    atomicStore(&local_index_internal, 0u);
  }
  workgroupBarrier();

  let global_x = global_id.x % (1024/16);
  let global_y = global_id.x / (1024/16);
  
  let i0 = global_y * (1024/16 + 1) + global_x;
  let i1 = (global_y + 1) * (1024/16 + 1) + global_x;
  let i2 = (global_y + 1) * (1024/16 + 1) + (global_x + 1);
  let i3 = global_y * (1024/16 + 1) + (global_x + 1);

  let p0 = vertices.v_vertices[i0];
  let p1 = vertices.v_vertices[i1];
  let p2 = vertices.v_vertices[i2];
  let p3 = vertices.v_vertices[i3];

  let output0 = p0.w == 1.0 && p1.w == 1.0 && p2.w == 1.0; 
  let output1 = p0.w == 1.0 && p2.w == 1.0 && p3.w == 1.0; 
  
  var add = 0u;
  if (output0) {
    add += 3;
  }
  if (output1) {
    add += 3;
  }

  let thread_index_internal = atomicAdd(&local_index_internal, add);

  workgroupBarrier();
      // One thread per group reserves a block in the global buffer
  var base : u32 = 0u;
  if (lid.x == 0u) {
      let count = atomicLoad(&local_index_internal);
      base = atomicAdd(&internal_indices.counter, count);
      // Share base with rest of workgroup
      atomicStore(&local_index_internal, base);
  }
  workgroupBarrier();
  // Read the base (now stored back into localCounter)
  let base_internal = atomicLoad(&local_index_internal);
   
  
  // // shading/lighting	
  //var col: vec3<f32> = vec3<f32>(0.0);
  // if( t<tmax )
  // {
  //     vec3<f32> pos = ro + t*rd;
  //     vec3<f32> nor = calcNormal(pos);
  //     f32 dif = clamp( dot(nor,vec3<f32>(0.57703)), 0.0, 1.0 );
  //     f32 amb = 0.5 + 0.5*dot(nor,vec3<f32>(0.0,1.0,0.0));
  //     col = vec3<f32>(0.2,0.3,0.4)*amb + vec3<f32>(0.8,0.7,0.5)*dif;
  // }

  // gamma        
  //col = sqrt( col );
  //tot += col;

  //atomicStore(&internal_indices.counter, 9u);
  internal_indices.instance_count = 1;
  internal_indices.first_index = 0;
  internal_indices.base_vertex = 0;
  internal_indices.first_instance = 0;

  let global_index = base_internal + thread_index_internal;
  add = 0u;
  if (output0) {
    internal_indices.v_indices[global_index + add] = i0;
    add++;
    internal_indices.v_indices[global_index + add] = i1;
    add++;
    internal_indices.v_indices[global_index + add] = i2;
    add++;
  }
  if (output1) {
    internal_indices.v_indices[global_index + add] = i0;
    add++;
    internal_indices.v_indices[global_index + add] = i2;
    add++;
    internal_indices.v_indices[global_index + add] = i3;
    add++;
  }
}