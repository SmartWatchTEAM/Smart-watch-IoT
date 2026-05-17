
// senior_health_watch_case_v1.scad
// Parametric prototype case for ESP32-S3 Super Mini + ST7735 + MAX30102 + GY-521 + TP4056 + 503040 LiPo
// Units: mm. Open with OpenSCAD. Set VIEW to: "assembly", "shell", "bezel", "backplate", "components".
    
$fn = 128;
VIEW = "shell";
SHOW_COMPONENTS = true;

// ==== Main dimensions ====
case_w = 50;    
case_h = 64;     
case_z = 20;    
corner_r = 10;
wall = 2.0;
bezel_z = 3.0;
back_z = 2.2;
tray_z = 1.6;
strap_w = 22;
strap_lug_gap = 22.5;
lug_w = 4.5; 
lug_h = 6.5;
lug_z = 4.5; 

// ==== DISPLAY MODULE ====

// PCB tổng của module màn hình
// [width, height, thickness]
display_pcb = [31.38, 48.56, 3.0];

// Kính LCD / toàn bộ phần panel hiển thị
// dùng để kiểm tra va chạm cơ khí
lcd_panel = [30.07, 37.43, 2.2];

// Viewing Area (VA)
// vùng nhìn thấy khi màn sáng
// đây là kích thước nên dùng cho cửa sổ bezel
VA_lcd = [29.37, 34.33, 0.8];

// Active Area (AA)
// vùng pixel thật
// thường không dùng trực tiếp để cắt bezel
AA_lcd = [27.97, 32.63, 0.8];

// ==== MAIN BOARDS ====

// ESP32-S3 Super Mini
esp32_s3    = [25, 18.2, 4.5];

// GY-521 MPU6050
gy521       = [21.8, 17.0, 3.0];

// TP4056 USB-C charging board
tp4056      = [25.0, 19.0, 4.0];

// pin lipo
battery     = [54.0, 34.0, 4.0];     // 403450 

// cảm biến nhịp tim
max30102mod = [20.3, 15.5, 3.5];     

// mini speaker
speaker     = [12.0, 12.0, 3.0];

module rr2d(w,h,r){
    offset(r=r) offset(delta=-r) square([w,h], center=true);
}
module rounded_box(w,h,z,r){
    linear_extrude(height=z) rr2d(w,h,r);
}
module screw_hole(x,y,z=30){
    translate([x,y,-1]) cylinder(h=z, d=2.25);
}

module screw_boss_bottom(x, y, zpos, h=4.5, od=5.5, pilot_d=1.7) {
    translate([x, y, zpos])
        difference() {
            cylinder(h = h, d = od);
            translate([0, 0, -1])
                cylinder(h = h + 2, d = pilot_d);
        }
}
module strap_lugs(zbase=0){
    for(dir=[1, -1]) {
        // Xác định tâm của ngàm (và cũng là tâm của lỗ xỏ chốt)
        y_center = dir * (case_h/2 + lug_h/2 - 0.5);
        
        for(x=[-(strap_lug_gap/2+lug_w/2), (strap_lug_gap/2+lug_w/2)]){
            
            difference() {
                // Dùng lệnh hull() bọc 1 hình trụ ở ngoài và 1 mặt phẳng bám vào thân vỏ 
                // để tạo ra dáng ngàm chữ D (D-shape) bo tròn hoàn toàn
                hull() {
                    // Phần đuôi ngàm là hình trụ tròn (tâm trùng với chốt)
                    translate([x, y_center, zbase + lug_z/2]) 
                        cylinder(h=lug_z, d=lug_w, center=true);
                    
                    // Phần gốc ngàm bám sát vào mặt vỏ đồng hồ
                    translate([x, y_center - dir * (lug_h/2 - 0.5), zbase + lug_z/2]) 
                        cube([lug_w, 0.1, lug_z], center=true);
                }
                
                // Đục lỗ xỏ chốt dây đeo lò xo (đường kính 1.8mm chống kẹt)
                translate([x, y_center, zbase + lug_z/2]) 
                    rotate([0,90,0]) 
                        cylinder(h=lug_w+2, d=1.8, center=true);
            }
            
        }
    }
}

module shell_body() {
    difference() {
        union() {
            // Khối vỏ ngoài bây giờ cao lên tận đỉnh màn hình (case_z = 20)
            rounded_box(case_w, case_h, case_z, corner_r);
            strap_lugs(0);
        }

        // 1. Khoang rỗng bên trong (đục từ dưới bụng lên, chừa lại cái "nóc" dày 3mm)
        translate([0, 0, -0.1])
            rounded_box(
                case_w - 2 * wall,
                case_h - 2 * wall,
                case_z - bezel_z + 0.1,
                corner_r - wall
            );

        // 2. Cửa sổ màn hình (khoét xuyên qua lớp nóc trên cùng)
        translate([0, 4, case_z - bezel_z - 0.2])
            rounded_box(lcd_panel[0], lcd_panel[1], bezel_z + 1, 3.0);

        // 3. Hốc chứa ngàm PCB màn hình (nằm kề ngay dưới cửa sổ)
        translate([0, 4, case_z - bezel_z - 1.8])
            rounded_box(display_pcb[0] + 0.6, display_pcb[1] + 0.6, 2.2, 2.0);

        // 4. Cổng sạc Type-C
        translate([0, -case_h / 2 - 0.1, 8.2]) {
            rotate([90, 0, 0]) {
                hull() {
                    translate([-3.25, 0, 0]) cylinder(h = 5.0, d = 4.5, center = true);
                    translate([3.25, 0, 0]) cylinder(h = 5.0, d = 4.5, center = true);
                }
            }
        }

        // 5. Nút nguồn bên phải
        translate([case_w / 2 + 0.05, 5, 10.5])
            rotate([0, 90, 0]) cylinder(h = 5, d = 6.0, center = true);
        
        translate([case_w / 2 + 0.05, 13, 10.5])
            rotate([0, 90, 0]) cylinder(h = 5, d = 6.0, center = true);

        // 6. Nút SOS bên trái
        translate([-case_w / 2 - 0.05, -7, 10.5])
            rotate([0, 90, 0]) cylinder(h = 5, d = 8.5, center = true);

        // 7. Lỗ loa ở cạnh phải phía trên
        for (i = [-1, 0, 1])
            translate([case_w / 2 + 0.05, 22 + i * 3, 10.0])
                rotate([0, 90, 0]) cylinder(h = 5, d = 1.2, center = true);
    }

    // CHỈ GIỮ LẠI: screw boss phía dưới cho nắp lưng backplate
    // (Đã dọn dẹp sạch sẽ phần cọc vít đỉnh vì vỏ đã liền khối)
    for (x = [-(case_w / 2 - 6), (case_w / 2 - 6)])
        for (y = [-(case_h / 2 - 6), (case_h / 2 - 6)])
            screw_boss_bottom(x, y, 0);

    // Gân giữ pin
    translate([-19.5, 0, 5.0]) 
        cube([9, 45, 1.0], center = true); // Thanh ray bên trái
        
    translate([19.5, 0, 5.0]) 
        cube([9, 45, 1.0], center = true); // Thanh ray bên phải
}


module backplate() {
    difference() {
        translate([0, 0, -back_z])
            rounded_box(case_w, case_h, back_z, corner_r);

        // 1. Cửa sổ cảm biến MAX30102 (Lỗ hình chữ nhật 6x3 mm)
        translate([0, -2, -back_z - 0.5])
            linear_extrude(height = back_z + 1.5)
                square([6.0, 3.0], center = true);

        // lỗ vít nắp lưng
        for (x = [-(case_w / 2 - 6), (case_w / 2 - 6)])
            for (y = [-(case_h / 2 - 6), (case_h / 2 - 6)]){
                // lỗ xuyên vít M2
                translate([x, y, -back_z - 1])
                    cylinder(h = back_z + 4, d = 2.4);

                // countersink cho đầu vít
                translate([x, y, -back_z - 0.01])
                    cylinder(h = 1.2, d1 = 4.8, d2 = 2.4);
            }
    }

    // đệm cảm biến
    translate([0, -2, -back_z - 0.6])
        difference() {
            cylinder(h = 0.8, d = 15.0);
            translate([0, 0, -0.1]) cylinder(h = 1.1, d = 10.2);
        }
}

module comp_box(size, loc, col=[0.3,0.3,0.3,0.45]){
    color(col) translate(loc) cube(size, center=true);
}
module components() {
    // màn hình: đúng các lớp bạn khai báo
    comp_box(display_pcb, [0, 4, case_z - 4.6], [0.05, 0.05, 0.05, 0.40]);
    comp_box(lcd_panel,   [0, 4, case_z - 3.0], [0.10, 0.40, 0.90, 0.65]);
    comp_box(VA_lcd,      [0, 4, case_z - 3.0], [0.10, 0.80, 0.90, 0.35]);
    comp_box(AA_lcd,      [0, 4, case_z - 3.0], [0.10, 0.95, 0.30, 0.25]);

    // các board khác
    comp_box(esp32_s3,    [-11, 10, 9.7], [0.00, 0.45, 0.80, 0.55]);
    comp_box(gy521,       [-11, -16, 9.7], [0.85, 0.55, 0.05, 0.55]);
    comp_box(tp4056,      [8, -18, 9.6], [0.10, 0.75, 0.20, 0.55]);
    comp_box(speaker,     [14, 21, 9.6], [0.85, 0.15, 0.15, 0.60]);

    // pin LiPo: giữ nguyên kích thước đã khai báo, chỉ xoay để vừa case 44 x 60
    color([0.55, 0.55, 0.55, 0.50])
        translate([0, 0, 4.0])
            rotate([0, 0, 90])
                cube(battery, center = true);

    comp_box(max30102mod, [0, -2, 0.8], [0.65, 0.05, 0.05, 0.60]);
}

if (VIEW == "shell") {
    // Tự động lật úp bề mặt màn hình xuống bàn in Z=0 để bề mặt trơn láng nhất
    translate([0, 0, case_z])
        rotate([180, 0, 0])
            shell_body();
} 
else if (VIEW == "backplate") {
    // Nâng backplate lên mặt phẳng in (Z=0)
    translate([0, 0, back_z]) 
        backplate();
} 

