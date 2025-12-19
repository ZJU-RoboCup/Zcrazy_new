import QtQuick
import QtQuick.Controls

Item{
    id: uiRoot
    // 由父级（Main 的 Flickable）设置 width，这里只提供合理的隐式尺寸供滚动计算
    implicitWidth: 640
    implicitHeight: groupBox2.implicitHeight + 20
    property var cmdSender
    property var needChangeTeamState
    // 暴露在根上，供外部调用：设置当前 RobotID
    function setRobot(rid){ crazyShow.robotID = rid }

    //下面大的Box
    ZGroupBox{
        title : qsTr("66666") ;
        width: uiRoot.width - 15;
        anchors.top: uiRoot.top;
        anchors.topMargin:8;
        anchors.horizontalCenter: uiRoot.horizontalCenter;
        id : groupBox2;
        Grid{
            id : crazyShow;
            columns: 6;//6列
            verticalItemAlignment: Grid.AlignVCenter;
            horizontalItemAlignment: Grid.AlignLeft;
            anchors.horizontalCenter: parent.horizontalCenter;
            columnSpacing: 10;
            rowSpacing: 5;
            property int robotID : 0;//Robot
            property int velX : 0;//Vx
            property int velY : 0;//Vy
            property int velR : 0;//Vr
            property bool shoot : false;//Shoot
            property bool dribble : false;//Dribb
            property bool rush : false;//Rush

            property bool test_dribble : false;
            property int test_drib_level : 30;
            property int test_drib_vr : 0;
            property int test_drib_vr_step : 200;

            property int velXStep : 20;//VxStep
            property int velYStep : 20;//VyStep
            property int velRStep : 20;//VrStep
            property int angleStep : 10;
            property bool mode : false;//KickMode
            property int dribbleLevel : 10;//DribLevel
            property int rushSpeed : 20;//RushSpeed

            property int power : 20;//KickPower

            property int m_VELR : 8000;//MaxVelR [100rad/s]
            property int m_VEL : 800//MaxVel
            property int velocityRMax : m_VELR;//MaxVelR [100rad/s]
            property int velocityMax : m_VEL;//最大速度
            property int m_angle :180;
            property int angleMax : m_angle;//最大角度

            property int dribbleMaxLevel : 30;//吸球最大等级
            property double dribble_test_L : 98.72/1000.0;
            property double dribble_test_cos : 0.9725;
            property double dribble_test_sin : 0.2330;
            property int kickPowerMax: 200;//最大踢球力量[50us]
            property double r_VEL_RATIO : 0.01;
            property double r_VELR_RATIO : 0.01;
            property double r_DRIBBLE_RATIO : 0.1;
            property double r_KICK_RATIO : 50;
            // 每行有 (columns-1) 个缝隙
            property int itemWidth : (parent.width - columnSpacing * (columns - 1)) / columns;
            property string textV : qsTr("(cm/s)");
            property string textW : qsTr("(100rad/s)");
            property string textP : qsTr("(50us)");

            property bool use_imu : false;
            property double angle : 0;

            property bool only_one : true;
            property bool need_change_team : false;
            property bool control_all : false;
            property bool control_all_which_team : false; // 0 for blue,1 for yellow

            ZText{ text:qsTr("Robot")  }
            SpinBox{ editable:true; from:0; to:15; value:parent.robotID; width:parent.itemWidth
                onValueModified:{parent.robotID = value}}
            ZText{ text:"Stop" }
            Button{ text:qsTr("[Space]") ;width:parent.itemWidth }
            ZText{ text:" " }
            ZText{ text:" " }
            ZText{ text:qsTr("Vx [W/S]")  }
            SpinBox{ editable:true; from:-crazyShow.m_VEL; to:crazyShow.m_VEL; value:parent.velX;width:parent.itemWidth
                onValueModified:{parent.velX = value;}}
            ZText{ text:qsTr("VxStep "+parent.textV)  }
            SpinBox{ editable:true; from:1; to:crazyShow.m_VEL; value:parent.velXStep;width:parent.itemWidth;
                onValueModified:{parent.velXStep = value;}}
            ZText{ text:qsTr("MaxVel "+parent.textV)  }
            SpinBox{ editable:true; from:1; to:crazyShow.velocityMax; value:parent.m_VEL;width:parent.itemWidth
                onValueModified:{parent.m_VEL = value;}}
            ZText{ text:qsTr("Vy [A/D]") }
            SpinBox{ editable:true; from:-crazyShow.m_VEL; to:crazyShow.m_VEL; value:-parent.velY;width:parent.itemWidth
                onValueModified:{parent.velY = -value;}}
            ZText{ text:qsTr("VyStep "+parent.textV)  }
            SpinBox{ editable:true; from:1; to:crazyShow.m_VEL; value:parent.velYStep;width:parent.itemWidth
                onValueModified:{parent.velYStep = value;}}
            ZText{ text:" " }
            ZText{ text:" " }

            ZText{ text:qsTr(((parent.use_imu)?("angle"):("Vr")) +" [Left/Right]")  }
            SpinBox{ editable:true; from:(parent.use_imu)?(-crazyShow.m_angle):(-crazyShow.m_VELR); to:(parent.use_imu)?(crazyShow.m_angle):(crazyShow.m_VELR); value:(parent.use_imu)?(parent.angle):(parent.velR);width:parent.itemWidth
                onValueModified:{
                    if(parent.use_imu){ parent.angle = value; }
                    else{ parent.velR = value; }
                }}
            ZText{ text:qsTr((parent.use_imu)?("angleStep"):("VrStep "+parent.textW))  }
            SpinBox{ editable:true; from:1; to:(parent.use_imu)?(crazyShow.m_angle):(crazyShow.m_VELR); value:(parent.use_imu)?(parent.angleStep):(parent.velRStep);width:parent.itemWidth
                onValueModified:{ if(parent.use_imu){ parent.angleStep = value; } else { parent.velRStep = value; } }}
            ZText{ text:qsTr((parent.use_imu)?("MaxAngle"):("MaxVelR "+parent.textW))  }
            SpinBox{ editable:true; from:1; to:((parent.use_imu)?(crazyShow.angleMax):(crazyShow.velocityRMax)); value:((parent.use_imu)?(parent.m_angle):(parent.m_VELR));width:parent.itemWidth
                onValueModified:{ if(parent.use_imu){ parent.m_angle = value; } else { parent.m_VELR = value; } }}

            ZText{ text:qsTr("Shoot [E]") }
            Button{ text:(parent.shoot? qsTr("true") : qsTr("false")) ;width:parent.itemWidth
                onClicked: { parent.shoot = !parent.shoot; } }

            ZText{ text:qsTr("KickMode [Up]")  }
            Button{ text:(parent.mode?qsTr("chip"):qsTr("flat")) ;width:parent.itemWidth
                onClicked: { parent.mode = !parent.mode } }
            ZText{ text:qsTr("KickPower "+parent.textP)  }
            SpinBox{ editable:true; from:0; to:parent.kickPowerMax; value:parent.power;width:parent.itemWidth
                onValueModified:{parent.power = value;}}
            ZText{ text:qsTr("Dribb [Q]")  }
            Button{ text:(parent.dribble ? qsTr("true") : qsTr("false")) ;width:parent.itemWidth
                onClicked: { parent.dribble = !parent.dribble; } }
            ZText{ text:qsTr("DribLevel")  }
            SpinBox{ editable:true; from:0; to:crazyShow.dribbleMaxLevel; value:parent.dribbleLevel;width:parent.itemWidth
                onValueModified:{parent.dribbleLevel = value;}}
            ZText{ text:" " }
            ZText{ text:" " }
            ZText{ text:qsTr("Rush [G]")  }
            Button{ text:(parent.rush ? qsTr("true") : qsTr("false")) ;width:parent.itemWidth;
                onClicked: { parent.rush = !parent.rush; crazyShow.updateRush(); } }
            ZText{ text:qsTr("RushSpeed "+parent.textV)  }
            SpinBox{ editable:true; from:0; to:crazyShow.m_VEL; value:parent.rushSpeed;width:parent.itemWidth
                onValueModified:{parent.rushSpeed = value;}}
            Rectangle{ width:parent.itemWidth; height:20; color:parent.shoot ? "red" : "lightgrey"; }

            ZText{ text:" " }
            ZText{ text:qsTr("test_dribble [B]")  }
            Button{ text:(parent.test_dribble ? qsTr("true") : qsTr("false")) ;width:parent.itemWidth;
                onClicked: { parent.test_dribble = !parent.test_dribble; crazyShow.action_drib_test(); } }
            ZText{ text:qsTr("testVr [J K]"+parent.textW)  }
            SpinBox{ editable:true; from:-crazyShow.m_VELR; to:crazyShow.m_VELR; value:parent.test_drib_vr;width:parent.itemWidth
                onValueModified:{parent.test_drib_vr = value;}}
            ZText{ text:qsTr("testVrStep "+parent.textW)  }
            SpinBox{ editable:true; from:0; to:crazyShow.m_VELR; value:parent.test_drib_vr_step;width:parent.itemWidth
                onValueModified:{parent.test_drib_vr_step = value;}}

            ZText{ text:qsTr("use_imu [I]")  }
            Button{ text:(parent.use_imu ? qsTr("true") : qsTr("false")) ;width:parent.itemWidth;
                onClicked: { parent.use_imu = !parent.use_imu; } }

            ZText{ text:qsTr("only one [P]")  }
            Button{ text:(parent.only_one ? qsTr("true") : qsTr("false")) ;width:parent.itemWidth
                onClicked: { parent.only_one = !parent.only_one; infoViewer.car_num(only_one) } }

            ZText{ text:qsTr("change team [T]")  }
            Button{ text:(needChangeTeamState ? qsTr("true") : qsTr("false")) ;width:parent.itemWidth }

            ZText{ text:qsTr("control all [N]")  }
            Button{ text:(parent.control_all ? qsTr("true") : qsTr("false")) ;width:parent.itemWidth
                onClicked: { parent.control_all = !parent.control_all; } }

            ZText{ text:qsTr("control team [M]")  }
            Button{ text:qsTr((parent.control_all_which_team)?"yellow":"blue") ;width:parent.itemWidth
                onClicked: { parent.control_all_which_team = !parent.control_all_which_team; } }

            Keys.onPressed: (event) => {getFocus(event);}
            function getFocus(event){
                switch(event.key){
                case Qt.Key_Enter:
                case Qt.Key_Return:
                case Qt.Key_Escape:
                    crazyShow.focus = true;
                    break;
                default:
                    event.accepted = false;
                    return false;
                }
                event.accepted = true;
            }
            function updateStop(){
                crazyShow.velX = 0; crazyShow.velY = 0; crazyShow.velR = 0;
                crazyShow.shoot = false; crazyShow.dribble = false; crazyShow.rush = false;
            }
            function updateRush(){
                if(crazyShow.rush){ crazyShow.velX = crazyShow.rushSpeed; crazyShow.velY = 0; crazyShow.velR = 0; crazyShow.shoot = true; crazyShow.dribble = false; }
                else{ crazyShow.updateStop(); }
            }
            function action_drib_test(){
                if(crazyShow.test_dribble){
                    crazyShow.velY = -crazyShow.velR*crazyShow.dribble_test_L*crazyShow.dribble_test_cos;
                    crazyShow.dribbleLevel =crazyShow.test_drib_level;
                }else{ crazyShow.updateStop(); }
            }
            function handleKeyboardEvent(e){
                switch(e){
                case 'U':{crazyShow.mode = !crazyShow.mode;break;}
                case 'b':{crazyShow.test_dribble = !crazyShow.test_dribble; break;}
                case 'a':{crazyShow.velY = crazyShow.limitVel(crazyShow.velY+crazyShow.velYStep,-crazyShow.m_VEL,crazyShow.m_VEL); break;}
                case 'd':{crazyShow.velY = crazyShow.limitVel(crazyShow.velY-crazyShow.velYStep,-crazyShow.m_VEL,crazyShow.m_VEL); break;}
                case 'j':{crazyShow.velR = crazyShow.limitVel(crazyShow.velR-crazyShow.test_drib_vr_step,-crazyShow.m_VELR,crazyShow.m_VELR); action_drib_test(); break;}
                case 'k':{crazyShow.velR = crazyShow.limitVel(crazyShow.velR+crazyShow.test_drib_vr_step,-crazyShow.m_VELR,crazyShow.m_VELR); action_drib_test(); break;}
                case 'w':{crazyShow.velX = crazyShow.limitVel(crazyShow.velX+crazyShow.velXStep,-crazyShow.m_VEL,crazyShow.m_VEL); break;}
                case 's':{crazyShow.velX = crazyShow.limitVel(crazyShow.velX-crazyShow.velXStep,-crazyShow.m_VEL,crazyShow.m_VEL); break;}
                case 'q':{crazyShow.dribble = !crazyShow.dribble; break;}
                case 'e':{crazyShow.shoot = !crazyShow.shoot; break;}
                case 'L':{
                    if(crazyShow.use_imu){
                        if (crazyShow.angle+crazyShow.angleStep > crazyShow.m_angle){ crazyShow.angle = crazyShow.angle - 2*crazyShow.m_angle; }
                        else{ crazyShow.angle = crazyShow.angle+crazyShow.angleStep }
                    } else { crazyShow.velR = crazyShow.limitVel(crazyShow.velR+crazyShow.velRStep,-crazyShow.m_VELR,crazyShow.m_VELR); }
                    break;}
                case 'R':{
                    if(crazyShow.use_imu){
                        if (crazyShow.angle-crazyShow.angleStep < -crazyShow.m_angle){ crazyShow.angle = crazyShow.angle + 2*  crazyShow.m_angle; }
                        else{ crazyShow.angle = crazyShow.angle-crazyShow.angleStep }
                    } else { crazyShow.velR = crazyShow.limitVel(crazyShow.velR-crazyShow.velRStep,-crazyShow.m_VELR,crazyShow.m_VELR); }
                    break;}
                case 'S':{crazyShow.updateStop(); break;}
                case 'g':{crazyShow.rush = !crazyShow.rush; updateRush(); break;}
                case 'i':{ crazyShow.use_imu = !crazyShow.use_imu; break; }
                case 'p':{ crazyShow.only_one = !crazyShow.only_one; infoViewer.car_num(only_one); break; }
                case 'N':{ crazyShow.control_all = !crazyShow.control_all; break; }
                case 'M':{ crazyShow.control_all_which_team = !crazyShow.control_all_which_team; break; }
                default: return false;
                }
                updateCommand();
            }
            function updateCommand(){
                cmdSender.updateCommandParams(crazyShow.robotID,
                    crazyShow.velX*crazyShow.r_VEL_RATIO,
                    crazyShow.velY*crazyShow.r_VEL_RATIO,
                    crazyShow.velR*crazyShow.r_VELR_RATIO,
                    (crazyShow.dribble?crazyShow.dribbleLevel:0)*crazyShow.r_DRIBBLE_RATIO,
                    crazyShow.mode,
                    crazyShow.shoot,
                    crazyShow.power*crazyShow.r_KICK_RATIO,
                    crazyShow.use_imu,
                    crazyShow.angle,
                    crazyShow.control_all,
                    crazyShow.control_all_which_team
                );
            }
            function updateFromGamepad(){ /* no-op for now */ }
            function limitVel(vel,minValue,maxValue){ if(vel>maxValue) return maxValue; if(vel<minValue) return minValue; return vel; }
            Shortcut{ sequence:"G"; onActivated:crazyShow.handleKeyboardEvent('g'); }
            Shortcut{ sequence:"A"; onActivated:crazyShow.handleKeyboardEvent('a'); }
            Shortcut{ sequence:"Up"; onActivated:crazyShow.handleKeyboardEvent('U'); }
            Shortcut{ sequence:"D"; onActivated:crazyShow.handleKeyboardEvent('d'); }
            Shortcut{ sequence:"W"; onActivated:crazyShow.handleKeyboardEvent('w'); }
            Shortcut{ sequence:"S"; onActivated:crazyShow.handleKeyboardEvent('s'); }
            Shortcut{ sequence:"Q"; onActivated:crazyShow.handleKeyboardEvent('q'); }
            Shortcut{ sequence:"E"; onActivated:crazyShow.handleKeyboardEvent('e'); }
            Shortcut{ sequence:"Left"; onActivated:crazyShow.handleKeyboardEvent('L'); }
            Shortcut{ sequence:"Right"; onActivated:crazyShow.handleKeyboardEvent('R'); }
            Shortcut{ sequence:"Space"; onActivated:crazyShow.handleKeyboardEvent('S'); }
            Shortcut{ sequence:"B"; onActivated:crazyShow.handleKeyboardEvent('b'); }
            Shortcut{ sequence:"I"; onActivated:crazyShow.handleKeyboardEvent('i'); }
            Shortcut{ sequence:"P"; onActivated:crazyShow.handleKeyboardEvent('p'); }
            Shortcut{ sequence:"N"; onActivated:crazyShow.handleKeyboardEvent('N'); }
            Shortcut{ sequence:"M"; onActivated:crazyShow.handleKeyboardEvent('M'); }
            Shortcut{ sequence:"J"; onActivated:crazyShow.handleKeyboardEvent('j'); }
            Shortcut{ sequence:"K"; onActivated:crazyShow.handleKeyboardEvent('k'); }
        }
    }
}
