#![allow(dead_code)]
#[path="../third_party/horizon/crates/horizon-client-input/src/mouse.rs"] mod mouse;
#[path="../.local/horizon-input-arithmetic.rs"] mod input;
use mouse::*;
fn main(){
    let mut m=MouseLook::default();let mut r=Rotation{x_rot:0.,y_rot:0.,x_rot_o:0.,y_rot_o:0.};
    for i in 0..4096 {
        if i%64==0{m=MouseLook::default();r=Rotation{x_rot:0.,y_rot:0.,x_rot_o:0.,y_rot_o:0.};}
        let o=LookOptions{sensitivity:(i%5) as f64/4.,smooth_camera:i&1!=0,scoping:i&2!=0,invert_x:i&4!=0,invert_y:i&8!=0};
        let dx=(i%41) as f64-20.25;let dy=(i%37) as f64-18.5;
        m.on_move(i as f64*1.25,-(i as f64)*0.75,dx,dy,i%17!=0,i%13!=0);
        if i%3==0{m.add_motion(dx*0.5,dy*0.25);}
        let t=m.handle_accumulated_movement(i as f64/60.,i%17!=0,i%13!=0,o);
        let valid=t.is_some();let (x,y)=t.unwrap_or((0.,0.));if valid{r.turn(x,y);}
        let wheel=m.on_scroll(if i%2==0{0.4}else{-0.7},if i%5==0{0.7}else{0.4},i%7==0,if i%11==0{0.5}else{1.});
        let (w,(wx,wy))=wheel.unwrap_or((0,(0,0)));
        println!("M {} {} {:016x} {:016x} {:08x} {:08x} {:08x} {:08x} {} {} {} {}",i,valid as u8,x.to_bits(),y.to_bits(),r.x_rot.to_bits(),r.y_rot.to_bits(),r.x_rot_o.to_bits(),r.y_rot_o.to_bits(),wheel.is_some() as u8,wx,wy,w);
    }
    for keys in 0..16 {for use_mode in 0..4 {for sneak_mode in 0..4 {
        let d=input::keyboard_direction(keys&1!=0,keys&2!=0,keys&4!=0,keys&8!=0);
        let factor=|mode|match mode{0=>None,1=>Some(0.2),2=>Some(0.3),_=>Some(0.5)};
        let v=input::modify_input(d,factor(use_mode),factor(sneak_mode));
        println!("K {} {} {} {:08x} {:08x} {:08x} {:08x}",keys,use_mode,sneak_mode,d[0].to_bits(),d[1].to_bits(),v[0].to_bits(),v[1].to_bits());
    }}}
}
