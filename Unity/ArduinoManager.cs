using UnityEngine;
using System.IO.Ports;
using System.Collections;
using System.Threading;
using System;

public class ArduinoManager : MonoBehaviour
{
    public string portName;
    public string message;
    public bool disableWeightSim;
    public bool disableEjection;
    private bool looping = true;
    private bool docked = false;
    private bool drainingAir = false;
    private string writeCue = "";
    private bool startDelayedCall = false;
    SerialPort sp;
    float next_time;

    private Thread thread;
    public enum FillLevel
    {
        Empty=0,
        VeryLow=1,
        Low=2,
        FairlyLow=3,
        LowerMedium=4,
        Medium=5,
        UpperMedium = 6,
        FarlyHigh =7,
        High=8,
        VeryHigh=9,
        Full=10
    }


    private FillLevel nextFillLevel = FillLevel.Empty;
    private FillLevel FillLevelLeft = FillLevel.Empty;
    private FillLevel FillLevelRight = FillLevel.Empty;


    void Start()
    {   if (disableWeightSim) { return; }
        string the_com = "";
        next_time = Time.time;

        foreach (string mysps in SerialPort.GetPortNames())
        {
            print(mysps);
            if (mysps == portName) { the_com = mysps; break; }
        }
        sp = new SerialPort("\\\\.\\" + the_com, 9600);
        if (!sp.IsOpen)
        {
            print("Opening " + the_com + ", baud 9600");
            sp.Open();
            sp.ReadTimeout =100;
            sp.Handshake = Handshake.None;
            if (sp.IsOpen) {
                print("Open");
                StartThread();
            }
            
        }

    }

    // Update is called once per frame
    void Update()
    {

        if (Input.GetKeyUp(KeyCode.L))
        {
            SendEvent("L5");
        }


        if (Input.GetKeyUp(KeyCode.DownArrow))
        {
            SendEvent("l5");
        }


        if (Input.GetKeyUp(KeyCode.H))
        {
            SendEvent("h".ToString());
        }


        if (Input.GetKeyUp(KeyCode.S))
        {
            SendEvent(message);
        }

        if (startDelayedCall)
        {
            startDelayedCall = false;
            StartCoroutine(DelayedCall(0.5f));

        }

    }

    void SendEvent(string s)
    {
        if (disableWeightSim) { return; }
        if (!sp.IsOpen)
        {
            sp.Open();
            print("opened sp");
        }
        if (sp.IsOpen)
        {
            print("Writing "+s);
            sp.Write((s));
        }
    }



    public FillLevel GetLeftFillLevel() { return FillLevelLeft; }
    public FillLevel GetRightFillLevel() { return FillLevelRight; }

    public void StartThread()
    {
        thread = new Thread(ThreadLoop);
        thread.Start();
    }

    public void StopThread()
    {
        lock (this)
        {
            looping = false;
        }
    }

    public bool IsLooping()
    {
        lock (this)
        {
            return looping;
        }
    }

    public void ThreadLoop()
    {
        while (IsLooping())
        {
           
            // Read from Arduino
            string result = sp.ReadLine().ToString();
            if (result == "C0")
            {
                if (!docked)
                {
                    docked = true;
                }
                
            } else if (result == "C1")
            {
                 docked = false;
            }
            else if (result == "FIN")
            {
                Debug.Log("FIN");

                if (!disableEjection)
                {
                    if (!drainingAir)
                    {
                        //eject controller
                        SendEvent("E");
                    }

                    else { startDelayedCall = true; }
                }
               
                
            }


        }
        sp.Close();
    }


    IEnumerator DelayedCall(float waitTime)
    {
        yield return new WaitForSeconds(waitTime);
        drainingAir = false;
        SendEvent(writeCue);
        
        Debug.Log("Now send the delayed call of" + writeCue);
        writeCue = "";

    }

    private void OnDestroy()
    {
        StopThread();
    }
    private void OnApplicationQuit()
    {
        StopThread();
    }

    public bool GetIsDocked()
    {
        //if (disableWeightSim) { return true; }
        return docked;
    }


    public void SetWeight(int lvlDiff, bool isEmpty)
    {
        String call = "";
        if (lvlDiff > 0)
        {
            call = "R".ToString() + ((int)lvlDiff-1);      
        }

        if (lvlDiff < 0)
        {
            lvlDiff = -lvlDiff;
            call = "r".ToString() + ((int)lvlDiff - 1);
        }

        Debug.Log("filter_ so call is " + call);

        if (!isEmpty)
        {
            SendEvent(call);
        } else
        {   
            // if controller is empty, pump out the air before sending the pump command
            drainingAir = true;
            writeCue = call;
            SendEvent("r2");
        }
        

    }

}
