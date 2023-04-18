#include "DisplayUI.h"
#include "settings.h"
void DisplayUI::configInit() 
{
    display.init();
    display.setFont(DejaVu_Sans_Mono_12);
    display.setContrast(255);
    if (FLIP_DIPLAY) display.flipScreenVertically();
    display.clear();
    display.display();
}

void DisplayUI::configOn() 
{
    display.displayOn();
}

void DisplayUI::configOff() 
{
    display.displayOff();
}

void DisplayUI::updatePrefix() 
{
    display.clear();
}

void DisplayUI::updateSuffix() 
{
    display.display();
}

void DisplayUI::drawString(int x, int y, String str) 
{
    display.drawString(x, y, replaceUtf8(str, String(QUESTIONMARK)));
}

void DisplayUI::drawString(int row, String str) 
{
    drawString(0, row * lineHeight, str);
}

void DisplayUI::drawLine(int x1, int y1, int x2, int y2) 
{
    display.drawLine(x1, y1, x2, y2);
}
DisplayUI::DisplayUI() {}
DisplayUI::~DisplayUI() {}
void DisplayUI::setup() 
{
    configInit();
    setupButtons();
    buttonTime = currentTime;

#ifdef RTC_DS3231
    bool h12;
    bool PM_time;
    clock.setClockMode(false);
    clockHour   = clock.getHour(h12, PM_time);
    clockMinute = clock.getMinute();
#else 
    clockHour   = random(12);
    clockMinute = random(60);
#endif 
    createMenu(&mainMenu, NULL, [this]()
    {
        addMenuNode(&mainMenu, D_SCAN, &scanMenu);         
        addMenuNode(&mainMenu, D_SHOW, &showMenu);        
        addMenuNode(&mainMenu, D_ATTACK, &attackMenu);    
        addMenuNode(&mainMenu, D_PACKET_MONITOR, [this]() 
        { 
            scan.start(SCAN_MODE_SNIFFER, 0, SCAN_MODE_OFF, 0, false, wifi_channel);
            mode = DISPLAY_MODE::PACKETMONITOR;
        });
        addMenuNode(&mainMenu, D_CLOCK, &clockMenu);

#ifdef HIGHLIGHT_LED
        addMenuNode(&mainMenu, D_LED, [this]() 
        {
            highlightLED = !highlightLED;
            digitalWrite(HIGHLIGHT_LED, highlightLED);
        });
#endif 
    });
    createMenu(&scanMenu, &mainMenu, [this]() 
    {
        addMenuNode(&scanMenu, D_SCAN_APST, [this]() 
        { 
            scan.start(SCAN_MODE_ALL, 15000, SCAN_MODE_OFF, 0, true, wifi_channel);
            mode = DISPLAY_MODE::LOADSCAN;
        });
        addMenuNode(&scanMenu, D_SCAN_AP, [this]() 
        {
            scan.start(SCAN_MODE_APS, 0, SCAN_MODE_OFF, 0, true, wifi_channel);
            mode = DISPLAY_MODE::LOADSCAN;
        });
        addMenuNode(&scanMenu, D_SCAN_ST, [this]() 
        { 
            scan.start(SCAN_MODE_STATIONS, 30000, SCAN_MODE_OFF, 0, true, wifi_channel);
            mode = DISPLAY_MODE::LOADSCAN;
        });
    });
    createMenu(&showMenu, &mainMenu, [this]() 
    {
        addMenuNode(&showMenu, [this]() 
        {
            return leftRight(str(D_ACCESSPOINTS), (String)accesspoints.count(), maxLen - 1);
        }, &apListMenu);
        addMenuNode(&showMenu, [this]()
        { 
            return leftRight(str(D_STATIONS), (String)stations.count(), maxLen - 1);
        }, &stationListMenu);
        addMenuNode(&showMenu, [this]() 
        {
            return leftRight(str(D_NAMES), (String)names.count(), maxLen - 1);
        }, &nameListMenu);
        addMenuNode(&showMenu, [this]() 
        {
            return leftRight(str(D_SSIDS), (String)ssids.count(), maxLen - 1);
        }, &ssidListMenu);
    });
    createMenu(&apListMenu, &showMenu, [this]() 
    {
        int c = accesspoints.count();
        for (int i = 0; i < c; i++)
        {
            addMenuNode(&apListMenu, [i]()
            {
                return b2a(accesspoints.getSelected(i)) + accesspoints.getSSID(i);
            }, [this, i]() 
            {
                accesspoints.getSelected(i) ? accesspoints.deselect(i) : accesspoints.select(i);
            }, [this, i]() 
            {
                selectedID = i;
                changeMenu(&apMenu);
            });
        }
        addMenuNode(&apListMenu, D_SELECT_ALL, [this]() 
        {
            accesspoints.selectAll();
            changeMenu(&apListMenu);
        });
        addMenuNode(&apListMenu, D_DESELECT_ALL, [this]() 
        {
            accesspoints.deselectAll();
            changeMenu(&apListMenu);
        });
        addMenuNode(&apListMenu, D_REMOVE_ALL, [this]() 
        { 
            accesspoints.removeAll();
            goBack();
        });
    });
    createMenu(&stationListMenu, &showMenu, [this]() 
    {
        int c = stations.count();
        for (int i = 0; i < c; i++)
        {
            addMenuNode(&stationListMenu, [i]()
            {
                return b2a(stations.getSelected(i)) +
                (stations.hasName(i) ? stations.getNameStr(i) : stations.getMacVendorStr(i));
            }, [this, i]() {
                stations.getSelected(i) ? stations.deselect(i) : stations.select(i);
            }, [this, i]() {
                selectedID = i;
                changeMenu(&stationMenu);
            });
        }

        addMenuNode(&stationListMenu, D_SELECT_ALL, [this]() 
        {
            stations.selectAll();
            changeMenu(&stationListMenu);
        });
        addMenuNode(&stationListMenu, D_DESELECT_ALL, [this]() 
        { 
            stations.deselectAll();
            changeMenu(&stationListMenu);
        });
        addMenuNode(&stationListMenu, D_REMOVE_ALL, [this]() 
        {
            stations.removeAll();
            goBack();
        });
    });
    createMenu(&nameListMenu, &showMenu, [this]() 
    {
        int c = names.count();

        for (int i = 0; i < c; i++)
        {
            addMenuNode(&nameListMenu, [i]() 
            {
                return names.getSelectedStr(i) + names.getName(i);
            }, [this, i]() 
            {
                names.getSelected(i) ? names.deselect(i) : names.select(i);
            }, [this, i]() 
            {
                selectedID = i;
                changeMenu(&nameMenu);
            });
        }
        addMenuNode(&nameListMenu, D_SELECT_ALL, [this]() 
        {
            names.selectAll();
            changeMenu(&nameListMenu);
        });
        addMenuNode(&nameListMenu, D_DESELECT_ALL, [this]() 
        { 
            names.deselectAll();
            changeMenu(&nameListMenu);
        });
        addMenuNode(&nameListMenu, D_REMOVE_ALL, [this]() 
        { 
            names.removeAll();
            goBack();
        });
    });
    createMenu(&ssidListMenu, &showMenu, [this]() 
    {
        addMenuNode(&ssidListMenu, D_CLONE_APS, [this]()
        { 
            ssids.cloneSelected(true);
            changeMenu(&ssidListMenu);
            ssids.save(false);
        });
        addMenuNode(&ssidListMenu, [this]()
        {
            return b2a(ssids.getRandom()) + str(D_RANDOM_MODE);
        }, [this]() {
            if (ssids.getRandom()) ssids.disableRandom();
            else ssids.enableRandom(10);
            changeMenu(&ssidListMenu);
        });
        int c = ssids.count();

        for (int i = 0; i < c; i++)
        {
            addMenuNode(&ssidListMenu, [i]() 
            {
                return ssids.getName(i).substring(0, ssids.getLen(i));
            }, [this, i]()
            {
                selectedID = i;
                changeMenu(&ssidMenu);
            }, [this, i]() 
            {
                ssids.remove(i);
                changeMenu(&ssidListMenu);
                ssidListMenu.selected = i;
            });
        }

        addMenuNode(&ssidListMenu, D_REMOVE_ALL, [this]() 
        { 
            ssids.removeAll();
            goBack();
        });
    });
    createMenu(&apMenu, &apListMenu, [this]()
    {
        addMenuNode(&apMenu, [this]() 
        {
            return accesspoints.getSelectedStr(selectedID)  + accesspoints.getSSID(selectedID); 
        }, [this]() 
        {
            accesspoints.getSelected(selectedID) ? accesspoints.deselect(selectedID) : accesspoints.select(selectedID);
        });
        addMenuNode(&apMenu, [this]() 
        {
            return str(D_ENCRYPTION) + accesspoints.getEncStr(selectedID);
        }, NULL);                                                                          
        addMenuNode(&apMenu, [this]() 
        {
            return str(D_RSSI) + (String)accesspoints.getRSSI(selectedID);
        }, NULL);                                                                        
        addMenuNode(&apMenu, [this]() 
        {
            return str(D_CHANNEL) + (String)accesspoints.getCh(selectedID);
        }, NULL);                                                                      
        addMenuNode(&apMenu, [this]() 
        {
            return accesspoints.getMacStr(selectedID);
        }, NULL);                                                                         
        addMenuNode(&apMenu, [this]() 
        {
            return str(D_VENDOR) + accesspoints.getVendorStr(selectedID);
        }, NULL);                                                                        
        addMenuNode(&apMenu, [this]() 
        {
            return accesspoints.getSelected(selectedID) ? str(D_DESELECT) : str(D_SELECT);
        }, [this]() 
        {
            accesspoints.getSelected(selectedID) ? accesspoints.deselect(selectedID) : accesspoints.select(selectedID);
        });
        addMenuNode(&apMenu, D_CLONE, [this]()
        { 
            ssids.add(accesspoints.getSSID(selectedID), accesspoints.getEnc(selectedID) != ENC_TYPE_NONE, 60, true);
            changeMenu(&showMenu);
            ssids.save(false);
        });
        addMenuNode(&apMenu, D_REMOVE, [this]() 
        { 
            accesspoints.remove(selectedID);
            apListMenu.list->remove(apListMenu.selected);
            goBack();
        });
    });
    createMenu(&stationMenu, &stationListMenu, [this]() 
    {
        addMenuNode(&stationMenu, [this]() 
        {
            return stations.getSelectedStr(selectedID) +
            (stations.hasName(selectedID) ? stations.getNameStr(selectedID) : stations.getMacVendorStr(selectedID));
        }, [this]() 
        {
            stations.getSelected(selectedID) ? stations.deselect(selectedID) : stations.select(selectedID);
        });
        addMenuNode(&stationMenu, [this]() 
        {
            return stations.getMacStr(selectedID);
        }, NULL);                                            
        addMenuNode(&stationMenu, [this]() 
        {
            return str(D_VENDOR) + stations.getVendorStr(selectedID);
        }, NULL);                                             
        addMenuNode(&stationMenu, [this]() 
        {
            return str(D_AP) + stations.getAPStr(selectedID);
        }, [this]() {
            int apID = accesspoints.find(stations.getAP(selectedID));

            if (apID >= 0) {
                selectedID = apID;
                changeMenu(&apMenu);
            }
        });
        addMenuNode(&stationMenu, [this]() 
        {
            return str(D_PKTS) + String(*stations.getPkts(selectedID));
        }, NULL);                                                                   
        addMenuNode(&stationMenu, [this]() 
        {
            return str(D_CHANNEL) + String(stations.getCh(selectedID));
        }, NULL);                                                                  
        addMenuNode(&stationMenu, [this]() 
        {
            return str(D_SEEN) + stations.getTimeStr(selectedID);
        }, NULL);                                                                      

        addMenuNode(&stationMenu, [this]() 
        {
            return stations.getSelected(selectedID) ? str(D_DESELECT) : str(D_SELECT);
        }, [this]() 
        {
            stations.getSelected(selectedID) ? stations.deselect(selectedID) : stations.select(selectedID);
        });
        addMenuNode(&stationMenu, D_REMOVE, [this]()
        {
            stations.remove(selectedID);
            stationListMenu.list->remove(stationListMenu.selected);
            goBack();
        });
    });

    // NAME MENU
    createMenu(&nameMenu, &nameListMenu, [this]() 
    {
        addMenuNode(&nameMenu, [this]() 
        {
            return names.getSelectedStr(selectedID) + names.getName(selectedID); 
        }, [this]() {
            names.getSelected(selectedID) ? names.deselect(selectedID) : names.select(selectedID);
        });
        addMenuNode(&nameMenu, [this]() 
        {
            return names.getMacStr(selectedID);
        }, NULL);                                                                 
        addMenuNode(&nameMenu, [this]() 
        {
            return str(D_VENDOR) + names.getVendorStr(selectedID);
        }, NULL);                                                               
        addMenuNode(&nameMenu, [this]() 
        {
            return str(D_AP) + names.getBssidStr(selectedID);
        }, NULL);                                                                   
        addMenuNode(&nameMenu, [this]() 
        {
            return str(D_CHANNEL) + (String)names.getCh(selectedID);
        }, NULL);                                                                 

        addMenuNode(&nameMenu, [this]() 
        {
            return names.getSelected(selectedID) ? str(D_DESELECT) : str(D_SELECT);
        }, [this]() 
        {
            names.getSelected(selectedID) ? names.deselect(selectedID) : names.select(selectedID);
        });
        addMenuNode(&nameMenu, D_REMOVE, [this]() 
        {
            names.remove(selectedID);
            nameListMenu.list->remove(nameListMenu.selected);
            goBack();
        });
    });
    createMenu(&ssidMenu, &ssidListMenu, [this]() 
    {
        addMenuNode(&ssidMenu, [this]() 
        {
            return ssids.getName(selectedID).substring(0, ssids.getLen(selectedID));
        }, NULL);                                            
        addMenuNode(&ssidMenu, [this]() 
        {
            return str(D_ENCRYPTION) + ssids.getEncStr(selectedID);
        }, [this]() 
        {
            ssids.setWPA2(selectedID, !ssids.getWPA2(selectedID));
        });
        addMenuNode(&ssidMenu, D_REMOVE, [this]() 
        {
            ssids.remove(selectedID);
            ssidListMenu.list->remove(ssidListMenu.selected);
            goBack();
        });
    });
    createMenu(&attackMenu, &mainMenu, [this]() 
    {
        addMenuNode(&attackMenu, [this]() 
        {
            if (attack.isRunning()) return leftRight(b2a(deauthSelected) + str(D_DEAUTH),
                                                     (String)attack.getDeauthPkts() + SLASH +
                                                     (String)attack.getDeauthMaxPkts(), maxLen - 1);
            else return leftRight(b2a(deauthSelected) + str(D_DEAUTH), (String)scan.countSelected(), maxLen - 1);
        }, [this]() 
        {
            deauthSelected = !deauthSelected;

            if (attack.isRunning()) 
            {
                attack.start(beaconSelected, deauthSelected, false, probeSelected, true,
                             settings::getAttackSettings().timeout * 1000);
            }
        });
        addMenuNode(&attackMenu, [this]() 
        {
            if (attack.isRunning()) return leftRight(b2a(beaconSelected) + str(D_BEACON),
                                                     (String)attack.getBeaconPkts() + SLASH +
                                                     (String)attack.getBeaconMaxPkts(), maxLen - 1);
            else return leftRight(b2a(beaconSelected) + str(D_BEACON), (String)ssids.count(), maxLen - 1);
        }, [this]() 
        {
            beaconSelected = !beaconSelected;

            if (attack.isRunning()) 
            {
                attack.start(beaconSelected, deauthSelected, false, probeSelected, true,
                             settings::getAttackSettings().timeout * 1000);
            }
        });
        addMenuNode(&attackMenu, [this]() 
        {
            if (attack.isRunning()) return leftRight(b2a(probeSelected) + str(D_PROBE),
                                                     (String)attack.getProbePkts() + SLASH +
                                                     (String)attack.getProbeMaxPkts(), maxLen - 1);
            else return leftRight(b2a(probeSelected) + str(D_PROBE), (String)ssids.count(), maxLen - 1);
        }, [this]() 
        {
            probeSelected = !probeSelected;

            if (attack.isRunning()) 
            {
                attack.start(beaconSelected, deauthSelected, false, probeSelected, true,
                             settings::getAttackSettings().timeout * 1000);
            }
        });
        addMenuNode(&attackMenu, [this]() 
        {
            return leftRight(str(attack.isRunning() ? D_STOP_ATTACK : D_START_ATTACK),
                             attack.getPacketRate() > 0 ? (String)attack.getPacketRate() : String(), maxLen - 1);
        }, [this]() {
            if (attack.isRunning()) attack.stop();
            else attack.start(beaconSelected, deauthSelected, false, probeSelected, true,
                              settings::getAttackSettings().timeout * 1000);
        });
    });
    createMenu(&clockMenu, &mainMenu, [this]() 
    {
        addMenuNode(&clockMenu, D_CLOCK_DISPLAY, [this]() 
        { 
            mode = DISPLAY_MODE::CLOCK_DISPLAY;
            display.setFont(ArialMT_Plain_24);
            display.setTextAlignment(TEXT_ALIGN_CENTER);
        });
        addMenuNode(&clockMenu, D_CLOCK_SET, [this]() 
        {
            mode = DISPLAY_MODE::CLOCK;
            display.setFont(ArialMT_Plain_24);
            display.setTextAlignment(TEXT_ALIGN_CENTER);
        });
    });
    changeMenu(&mainMenu);
    enabled   = true;
    startTime = currentTime;
}

#ifdef HIGHLIGHT_LED
void DisplayUI::setupLED() 
{
    pinMode(HIGHLIGHT_LED, OUTPUT);
    digitalWrite(HIGHLIGHT_LED, HIGH);
    highlightLED = true;
}
#endif
void DisplayUI::update(bool force) 
{
    if (!enabled) return;

    up->update();
    down->update();
    a->update();
    b->update();

    draw(force);

    uint32_t timeout = settings::getDisplaySettings().timeout * 1000;

    if (currentTime > timeout) 
    {
        if (!tempOff) 
        {
            if (buttonTime < currentTime - timeout) off();
        } else {
            if (buttonTime > currentTime - timeout) on();
        }
    }
}

void DisplayUI::on() 
{
    if (enabled) 
    {
        configOn();
        tempOff    = false;
        buttonTime = currentTime;
        prntln(D_MSG_DISPLAY_ON);
    } else 
    {
        prntln(D_ERROR_NOT_ENABLED);
    }
}

void DisplayUI::off() 
{
    if (enabled) 
    {
        configOff();
        tempOff = true;
        prntln(D_MSG_DISPLAY_OFF);
    } else {
        prntln(D_ERROR_NOT_ENABLED);
    }
}
void DisplayUI::setupButtons() 
{
    up   = new ButtonPullup(BUTTON_UP);
    down = new ButtonPullup(BUTTON_DOWN);
    a    = new ButtonPullup(BUTTON_A);
    b    = new ButtonPullup(BUTTON_B);
    up->setOnClicked([this]() 
    {
        scrollCounter = 0;
        scrollTime    = currentTime;
        buttonTime    = currentTime;

        if (!tempOff) 
        {
            if (mode == DISPLAY_MODE::MENU) 
            {
                if (currentMenu->selected > 0) currentMenu->selected--;
                else currentMenu->selected = currentMenu->list->size() - 1;
            } else if (mode == DISPLAY_MODE::PACKETMONITOR)
            { 
                scan.setChannel(wifi_channel + 1);
            } else if (mode == DISPLAY_MODE::CLOCK) 
            { 
                setTime(clockHour, clockMinute + 1, clockSecond);
            }
        }
    });

    up->setOnHolding([this]() 
    {
        scrollCounter = 0;
        scrollTime    = currentTime;
        buttonTime    = currentTime;
        if (!tempOff) 
        {
            if (mode == DISPLAY_MODE::MENU) 
            {
                if (currentMenu->selected > 0) currentMenu->selected--;
                else currentMenu->selected = currentMenu->list->size() - 1;
            } else if (mode == DISPLAY_MODE::PACKETMONITOR) 
            {
                scan.setChannel(wifi_channel + 1);
            } else if (mode == DISPLAY_MODE::CLOCK) 
            {
                setTime(clockHour, clockMinute + 10, clockSecond);
            }
        }
    }, buttonDelay);
    down->setOnClicked([this]() 
    {
        scrollCounter = 0;
        scrollTime    = currentTime;
        buttonTime    = currentTime;
        if (!tempOff) {
            if (mode == DISPLAY_MODE::MENU) 
            {
                if (currentMenu->selected < currentMenu->list->size() - 1) currentMenu->selected++;
                else currentMenu->selected = 0;
            } else if (mode == DISPLAY_MODE::PACKETMONITOR) 
            {
                scan.setChannel(wifi_channel - 1);
            } else if (mode == DISPLAY_MODE::CLOCK) 
            {
                setTime(clockHour, clockMinute - 1, clockSecond);
            }
        }
    });

    down->setOnHolding([this]() 
    {
        scrollCounter = 0;
        scrollTime    = currentTime;
        buttonTime    = currentTime;
        if (!tempOff) {
            if (mode == DISPLAY_MODE::MENU) 
            {
                if (currentMenu->selected < currentMenu->list->size() - 1) currentMenu->selected++;
                else currentMenu->selected = 0;
            } else if (mode == DISPLAY_MODE::PACKETMONITOR) 
            {
                scan.setChannel(wifi_channel - 1);
            }

            else if (mode == DISPLAY_MODE::CLOCK) 
            {
                setTime(clockHour, clockMinute - 10, clockSecond);
            }
        }
    }, buttonDelay);
    a->setOnClicked([this]() 
    {
        scrollCounter = 0;
        scrollTime    = currentTime;
        buttonTime    = currentTime;
        if (!tempOff) {
            switch (mode) 
            {
                case DISPLAY_MODE::MENU:

                    if (currentMenu->list->get(currentMenu->selected).click) {
                        currentMenu->list->get(currentMenu->selected).click();
                    }
                    break;

                case DISPLAY_MODE::PACKETMONITOR:
                case DISPLAY_MODE::LOADSCAN:
                    scan.stop();
                    mode = DISPLAY_MODE::MENU;
                    break;

                case DISPLAY_MODE::CLOCK:
                case DISPLAY_MODE::CLOCK_DISPLAY:
                    mode = DISPLAY_MODE::MENU;
                    display.setFont(DejaVu_Sans_Mono_12);
                    display.setTextAlignment(TEXT_ALIGN_LEFT);
                    break;
            }
        }
    });

    a->setOnHolding([this]() 
    {
        scrollCounter = 0;
        scrollTime    = currentTime;
        buttonTime    = currentTime;
        if (!tempOff) {
            if (mode == DISPLAY_MODE::MENU) 
            {
                if (currentMenu->list->get(currentMenu->selected).hold) 
                {
                    currentMenu->list->get(currentMenu->selected).hold();
                }
            }
        }
    }, 800);
    b->setOnClicked([this]()
    {
        scrollCounter = 0;
        scrollTime    = currentTime;
        buttonTime    = currentTime;
        if (!tempOff) 
        {
            switch (mode) 
            {
                case DISPLAY_MODE::MENU:
                    goBack();
                    break;

                case DISPLAY_MODE::PACKETMONITOR:
                case DISPLAY_MODE::LOADSCAN:
                    scan.stop();
                    mode = DISPLAY_MODE::MENU;
                    break;

                case DISPLAY_MODE::CLOCK:
                    mode = DISPLAY_MODE::MENU;
                    display.setFont(DejaVu_Sans_Mono_12);
                    display.setTextAlignment(TEXT_ALIGN_LEFT);
                    break;
            }
        }
    });
}

String DisplayUI::getChannel() 
{
    String ch = String(wifi_channel);

    if (ch.length() < 2) ch = ' ' + ch;
    return ch;
}

void DisplayUI::draw(bool force) 
{
    if (force || ((currentTime - drawTime > drawInterval) && currentMenu)) 
    {
        drawTime = currentTime;

        updatePrefix();

#ifdef RTC_DS3231
        bool h12;
        bool PM_time;
        clockHour   = clock.getHour(h12, PM_time);
        clockMinute = clock.getMinute();
        clockSecond = clock.getSecond();
#else
        if (currentTime - clockTime >= 1000) 
        {
            setTime(clockHour, clockMinute, ++clockSecond);
            clockTime += 1000;
        }
#endif

        switch (mode) 
        {
            case DISPLAY_MODE::BUTTON_TEST:
                drawButtonTest();
                break;

            case DISPLAY_MODE::MENU:
                drawMenu();
                break;

            case DISPLAY_MODE::LOADSCAN:
                drawLoadingScan();
                break;

            case DISPLAY_MODE::PACKETMONITOR:
                drawPacketMonitor();
                break;

            case DISPLAY_MODE::INTRO:
                if (!scan.isScanning() && (currentTime - startTime >= screenIntroTime)) {
                    mode = DISPLAY_MODE::MENU;
                }
                drawIntro();
                break;
            case DISPLAY_MODE::CLOCK:
            case DISPLAY_MODE::CLOCK_DISPLAY:
                drawClock();
                break;
            case DISPLAY_MODE::RESETTING:
                drawResetting();
                break;
        }

        updateSuffix();
    }
}

void DisplayUI::drawButtonTest() 
{
    drawString(0, str(D_UP) + b2s(up->read()));
    drawString(1, str(D_DOWN) + b2s(down->read()));
    drawString(2, str(D_A) + b2s(a->read()));
    drawString(3, str(D_B) + b2s(b->read()));
}

void DisplayUI::drawMenu() 
{
    String tmp;
    int    tmpLen;
    int    row = (currentMenu->selected / 5) * 5;
    if (currentMenu->selected < 0) currentMenu->selected = 0;
    else if (currentMenu->selected >= currentMenu->list->size()) currentMenu->selected = currentMenu->list->size() - 1;

    // draw menu entries
    for (int i = row; i < currentMenu->list->size() && i < row + 5; i++) 
    {
        tmp    = currentMenu->list->get(i).getStr();
        tmpLen = tmp.length();
        if ((currentMenu->selected == i) && (tmpLen >= maxLen)) 
        {
            tmp = tmp + tmp;
            tmp = tmp.substring(scrollCounter, scrollCounter + maxLen - 1);

            if (((scrollCounter > 0) && (scrollTime < currentTime - scrollSpeed)) || ((scrollCounter == 0) && (scrollTime < currentTime - scrollSpeed * 4))) {
                scrollTime = currentTime;
                scrollCounter++;
            }

            if (scrollCounter > tmpLen) scrollCounter = 0;
        }

        tmp = (currentMenu->selected == i ? CURSOR : SPACE) + tmp;
        drawString(0, (i - row) * 12, tmp);
    }
}

void DisplayUI::drawLoadingScan() 
{
    String percentage;
    if (scan.isScanning()) 
    {
        percentage = String(scan.getPercentage()) + '%';
    } else 
    {
        percentage = str(DSP_SCAN_DONE);
    }

    drawString(0, leftRight(str(DSP_SCAN_FOR), scan.getMode(), maxLen));
    drawString(1, leftRight(str(DSP_APS), String(accesspoints.count()), maxLen));
    drawString(2, leftRight(str(DSP_STS), String(stations.count()), maxLen));
    drawString(3, leftRight(str(DSP_PKTS), String(scan.getPacketRate()) + str(DSP_S), maxLen));
    drawString(4, center(percentage, maxLen));
}

void DisplayUI::drawPacketMonitor() 
{
    double scale = scan.getScaleFactor(sreenHeight - lineHeight - 2);

    String headline = leftRight(str(D_CH) + getChannel() + String(' ') + String('[') + String(scan.deauths) + String(']'), String(scan.getPacketRate()) + str(D_PKTS), maxLen);

    drawString(0, 0, headline);

    if (scan.getMaxPacket() > 0) 
    {
        int i = 0;
        int x = 0;
        int y = 0;

        while (i < SCAN_PACKET_LIST_SIZE && x < screenWidth) 
        {
            y = (sreenHeight-1) - (scan.getPackets(i) * scale);
            i++;

            // Serial.printf("%d,%d -> %d,%d\n", x, (sreenHeight-1), x, y);
            drawLine(x, (sreenHeight-1), x, y);
            x++;

            // Serial.printf("%d,%d -> %d,%d\n", x, (sreenHeight-1), x, y);
            drawLine(x, (sreenHeight-1), x, y);
            x++;
        }
        // Serial.println("---------");
    }
}

void DisplayUI::drawIntro() 
{
    drawString(0, center(str(D_INTRO_0), maxLen));
    drawString(1, center(str(D_INTRO_1), maxLen));
    drawString(2, center(str(D_INTRO_2), maxLen));
    drawString(3, center(DEAUTHER_VERSION, maxLen));
    if (scan.isScanning()) {
        if (currentTime - startTime >= screenIntroTime+4500) drawString(4, left(str(D_SCANNING_3), maxLen));
        else if (currentTime - startTime >= screenIntroTime+3000) drawString(4, left(str(D_SCANNING_2), maxLen));
        else if (currentTime - startTime >= screenIntroTime+1500) drawString(4, left(str(D_SCANNING_1), maxLen));
        else if (currentTime - startTime >= screenIntroTime) drawString(4, left(str(D_SCANNING_0), maxLen));
    }
}

void DisplayUI::drawClock() 
{
    String clockTime = String(clockHour);

    clockTime += ':';
    if (clockMinute < 10) clockTime += '0';
    clockTime += String(clockMinute);

    display.drawString(64, 20, clockTime);
}

void DisplayUI::drawResetting() 
{
    drawString(2, center(str(D_RESETTING), maxLen));
}

void DisplayUI::clearMenu(Menu* menu) 
{
    while (menu->list->size() > 0) 
    {
        menu->list->remove(0);
    }
}

void DisplayUI::changeMenu(Menu* menu) 
{
    if (menu) 
    {
        if (((menu == &apListMenu) && (accesspoints.count() == 0)) ||
            ((menu == &stationListMenu) && (stations.count() == 0)) ||
            ((menu == &nameListMenu) && (names.count() == 0))) {
            return;
        }

        if (currentMenu) clearMenu(currentMenu);
        currentMenu           = menu;
        currentMenu->selected = 0;
        buttonTime            = currentTime;

        if (selectedID < 0) selectedID = 0;

        if (currentMenu->parentMenu) 
        {
            addMenuNode(currentMenu, D_BACK, currentMenu->parentMenu); // add [BACK]
            currentMenu->selected = 1;
        }

        if (currentMenu->build) currentMenu->build();
    }
}

void DisplayUI::goBack() 
{
    if (currentMenu->parentMenu) changeMenu(currentMenu->parentMenu);
}

void DisplayUI::createMenu(Menu* menu, Menu* parent, std::function<void()>build) {
    menu->list       = new SimpleList<MenuNode>;
    menu->parentMenu = parent;
    menu->selected   = 0;
    menu->build      = build;
}

void DisplayUI::addMenuNode(Menu* menu, std::function<String()>getStr, std::function<void()>click,
                            std::function<void()>hold) {
    menu->list->add(MenuNode{ getStr, click, hold });
}

void DisplayUI::addMenuNode(Menu* menu, std::function<String()>getStr, std::function<void()>click) {
    addMenuNode(menu, getStr, click, NULL);
}

void DisplayUI::addMenuNode(Menu* menu, std::function<String()>getStr, Menu* next) {
    addMenuNode(menu, getStr, [this, next]() {
        changeMenu(next);
    });
}

void DisplayUI::addMenuNode(Menu* menu, const char* ptr, std::function<void()>click) {
    addMenuNode(menu, [ptr]() {
        return str(ptr);
    }, click);
}

void DisplayUI::addMenuNode(Menu* menu, const char* ptr, Menu* next) {
    addMenuNode(menu, [ptr]() {
        return str(ptr);
    }, next);
}

void DisplayUI::setTime(int h, int m, int s) {
    if (s >= 60) {
        s = 0;
        m++;
    }

    if (m >= 60) {
        m = 0;
        h++;
    }

    if (h >= 24) {
        h = 0;
    }

    if (s < 0) {
        s = 59;
        m--;
    }

    if (m < 0) {
        m = 59;
        h--;
    }

    if (h < 0) {
        h = 23;
    }

    clockHour   = h;
    clockMinute = m;
    clockSecond = s;

#ifdef RTC_DS3231
    clock.setHour(clockHour);
    clock.setMinute(clockMinute);
    clock.setSecond(clockSecond);
#endif
}
