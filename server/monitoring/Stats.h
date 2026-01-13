#pragma once

struct Stats { 
    double cpu; 
    long ram_used; 
    long ram_total; 
};

Stats getStats();
void cpu_monitor();
