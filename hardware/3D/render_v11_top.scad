$fn=10;

// The profile is 19.5 in height and 6mm in width
profile_height=17.5;
profile_width=6;
profile_top_part_start=7.75;

// Keep at 10 for developping, 100 for rendering 
fn_base=80;
// Geometric Parameters   
plastic_thickness=1.5;
profile_added_offset=1.5;
pcb_top_clearance=5.7;



module body(offset=0,fn_body,target_curve){
    //building the body by sliding the profile around
    minkowski() {
            linear_extrude(height = 0.001, center = false, convexity = 20)offset(r=offset-profile_width)import(file = "./DXF/MiniChord-User_front_plate_outline.dxf",$fn=fn_body*3);
            rotate_extrude()import(file = target_curve,$fn=fn_body/2);
        }
}




difference(){
    union(){
        //making the outer shell
        difference(){
            body(profile_added_offset,fn_base,"./DXF/MiniChord-User_top_edge_curve.dxf");
            translate([0,0,-plastic_thickness])body(-2,fn_base,"./DXF/MiniChord-User_top_edge_curve.dxf");
        }
        //adding the internal supports
        intersection(){
             body(profile_added_offset,fn_base,"./DXF/MiniChord-User_top_edge_curve.dxf");
            translate([0,0,-(profile_height)])linear_extrude(height =profile_height, center = false, convexity = 20)import (file = "./DXF/MiniChord-User_pcb_support.dxf",$fn=fn_base); 
        }
    }
        
    //>>the PCB cut
    pcb_thickness=1.6;
    //The PCB needs a 7.5mm clearance from the top interior of the shell
    
    //clear cut for component top 
    translate([0,0,-plastic_thickness-pcb_top_clearance])linear_extrude(height =pcb_top_clearance , center = false, convexity = 20)offset(r=0.20)import (file = "./DXF/MiniChord-User_top_component_clearance.dxf",$fn=fn_base); 
    //now the lead clearance
    translate([0,0,-plastic_thickness-pcb_top_clearance])linear_extrude(height =1.95 , center = false, convexity = 20)offset(r=0.25)import (file = "./DXF/MiniChord-User_led_clearance.dxf",$fn=fn_base); 
    
    //cut for button cutout
     translate([0,0,-profile_top_part_start])linear_extrude(height =profile_top_part_start+1 , center = false, convexity = 20)offset(r=0.25)import (file = "./DXF/MiniChord-User_front_plate_cutout.dxf",$fn=fn_base*2); 
     
      //cut for pot pins 
      translate([0,0,-plastic_thickness])linear_extrude(height =1 , center = false, convexity = 20)import (file = "./DXF/MiniChord-User_Drawings.dxf",$fn=fn_base*2); 
    
    
    //cut for connector
    connector_center_z=-plastic_thickness-pcb_top_clearance+1.45;
    height_connector_housing=6;
    height_connector=3;

     minkowski(){
         radius_connector=0.75;
         translate([0,0,connector_center_z-height_connector/2+radius_connector])linear_extrude(height =height_connector-2*radius_connector, center = false, convexity = 20)offset(r=-radius_connector)import (file = "./DXF/MiniChord-User_pcb_usb_connector.dxf");
          sphere(r=radius_connector,$fn=fn_base);
     }
     minkowski(){
     radius_housing=0.5;
    translate([0,0,connector_center_z-height_connector_housing/2-1])linear_extrude(height =height_connector_housing+1, center = false, convexity = 20)offset(r=-radius_housing)import (file = "./DXF/MiniChord-User_pcb_usb_housing.dxf"); 
   sphere(r=radius_housing,$fn=fn_base);
    }
    
   
    
    //cut for headphone
    //unfortunaly I have to do it manually, centered on the middle of the end of connector
    headphone_x=101.4452;
    headphone_y=-143.9548;
    headphone_center_z=-plastic_thickness-pcb_top_clearance+2.7;
    //connector
    translate([headphone_x,headphone_y,headphone_center_z])rotate([0,0,-45]){
        hull(){
        translate([0,3,0])rotate([90,0,0])cylinder(d=6.4,h=200,$fn=2*fn_base);
        translate([0,3,-5])rotate([90,0,0])cylinder(d=6.4,h=200,$fn=2*fn_base);
        }
    //translate([0,3,0])rotate([90,0,0])cylinder(d=8,h=1.7,$fn=2*fn_base);
    //translate([0,5,0])rotate([90,0,0])cylinder(d=8,h=3,$fn=2*fn_base);
    translate([0,3+5,0])cube([12,10,5],center=true);
    }
    
    
    //the screw hole cut, we offset the plate cutout to get them
    real_screw_d=3;
    target_screw_hole=1.9;//from 2.0 to have 2.3 screws, 1.9 is a bit bigger
    
    additional_depth=20;
     translate([0,0,-profile_top_part_start])linear_extrude(height =profile_top_part_start-1, center = false, convexity = 20)offset(r=(target_screw_hole-real_screw_d)/2)import(file = "./DXF/MiniChord-User_holes_housing.dxf",$fn=fn_base); 
    
}
