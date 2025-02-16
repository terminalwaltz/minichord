$fn=10;

// The profile is 19.5 in height and 6mm in width
profile_height=17.5;
profile_width=6;

// Keep at 10 for developping, 100 for rendering 
fn_base=80;

// Geometric Parameters   
profile_added_offset=1.5;
plastic_thickness=1.5;
pcb_top_clearance=5.7;
pcb_bottom_clearance=4;
pcb_thickness=1.6;
//The PCB needs a 7.5mm clearance from the top interior of the shell
pcb_depth=pcb_top_clearance+pcb_thickness;

module body(offset=0,fn_body,target_curve){
    //building the body by sliding the profile around
    minkowski() {
            linear_extrude(height = 0.001, center = false, convexity = 20)offset(r=offset-profile_width)import(file = "./DXF/MiniChord-User_front_plate_outline.dxf",$fn=fn_body*3);
            rotate_extrude()import(file = target_curve,$fn=fn_body/2);
        }
}

difference(){
    union(){
        //Making the shell
        difference(){
            //full shape
            body(profile_added_offset,fn_base,"./DXF/MiniChord-User_edge_curve.dxf");
            //minus the top shape 
            body(profile_added_offset,fn_base,"./DXF/MiniChord-User_bottom_edge_curve.dxf");
            //emptying the interior, 2mm bottom thickness
            translate([0,0,plastic_thickness])body(-3,fn_base,"./DXF/MiniChord-User_edge_curve.dxf");
            //removing the reset zone 
            translate([0,0,-profile_height])linear_extrude(height = profile_height, center = false, convexity = 20)import (file = "./DXF/MiniChord-User_reset_zone.dxf",$fn=3*fn_base); 
            
            
            
        }
        //Adding the PCB support
        intersection(){
            difference(){
                //full shape
                body(profile_added_offset,fn_base,"./DXF/MiniChord-User_edge_curve.dxf");
                //minus the top shape
                body(profile_added_offset,fn_base,"./DXF/MiniChord-User_bottom_edge_curve.dxf");
            }
            //intersecting with the PCB support profile
            translate([0,0,-(profile_height)])linear_extrude(height =profile_height, center = false, convexity = 20)import (file = "./DXF/MiniChord-User_pcb_support.dxf",$fn=fn_base); 
        }
        //adding the reset internal cover
        difference(){
            translate([0,0,-profile_height])linear_extrude(height = profile_height-pcb_depth-plastic_thickness, center = false, convexity = 20)offset(r=1)import (file = "./DXF/MiniChord-User_reset_zone.dxf",$fn=3*fn_base); //removing the reset zone 
            translate([0,0,-profile_height])linear_extrude(height = profile_height, center = false, convexity = 20)import (file = "./DXF/MiniChord-User_reset_zone.dxf",$fn=3*fn_base); 
        }
        //adding the battery holder
         translate([0,0,-profile_height])linear_extrude(height = 3+plastic_thickness, center = false, convexity = 20)import (file = "./DXF/MiniChord-User_battery_holder.dxf",$fn=fn_base); 
        /*translate([0,0,-profile_height+plastic_thickness+3])
        roof(method="straight", convexity = 10){
             import (file = "./DXF/MiniChord-User_battery_holder.dxf",$fn=fn_base);
        }*/
        
        
    }

    //>>the PCB cut
    //Let's cut the PCB  
    translate([0,0,-plastic_thickness-pcb_depth])linear_extrude(height = pcb_depth+plastic_thickness, center = false, convexity = 20)offset(r=0.1)import (file = "./DXF/MiniChord-Edge_Cuts.dxf",$fn=3*fn_base); 
    
    //>>The bottom clearance cut
    //We need to make room 
    added_depth=pcb_bottom_clearance+pcb_depth;
    translate([0,0,-plastic_thickness-added_depth])linear_extrude(height = added_depth+plastic_thickness, center = false, convexity = 20)offset(r=0.00)import (file = "./DXF/MiniChord-User_pcb_low_clear.dxf",$fn=fn_base/2); 
    
   
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
    translate([0,0,connector_center_z-height_connector_housing/2])linear_extrude(height =height_connector_housing+1, center = false, convexity = 20)offset(r=-0.5)import (file = "./DXF/MiniChord-User_pcb_usb_housing.dxf"); 
   sphere(r=0.5,$fn=fn_base);
    }
    
    //cut for headphone
    //unfortunaly I have to do it manually, centered on the middle of the end of connector
    headphone_x=101.4452;
    headphone_y=-143.9548;
    headphone_center_z=-plastic_thickness-pcb_top_clearance+2.7;
    //connector
    translate([headphone_x,headphone_y,headphone_center_z])rotate([0,0,-45]){
    translate([0,3,0])rotate([90,0,0])cylinder(d=6.4,h=200,$fn=2*fn_base);
    translate([0,3,0])rotate([90,0,0])cylinder(d=8,h=1.7,$fn=2*fn_base);
    translate([0,5,0])rotate([90,0,0])cylinder(d=8,h=3,$fn=2*fn_base);
    translate([0,2+5,0])cube([12,10,5],center=true);
    }
    
    
    //the screw hole cut, we offset the plate cutout to get them
    real_screw_d=3;
    target_screw_head=4.6;
    target_screw_body=2.9;

    additional_depth=20;
     translate([0,0,-profile_height])linear_extrude(height =profile_height, center = false, convexity = 20)offset(r=(target_screw_body-real_screw_d)/2)import(file = "./DXF/MiniChord-User_holes_housing.dxf",$fn=fn_base); 
    //let's stop 1mm before the pcb clearance
    translate([0,0,-profile_height])linear_extrude(height =profile_height-(plastic_thickness+pcb_depth), center = false, convexity = 20)offset(r=(target_screw_head-real_screw_d)/2)import(file = "./DXF/MiniChord-User_holes_housing.dxf",$fn=fn_base); 
    
   
    
}
