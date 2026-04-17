using System;
using System.Collections.Concurrent;
using System.Text;
using Newtonsoft.Json;
using System.Threading;
using System.Threading.Tasks;
using UnityEngine;
using MQTTnet;
using MQTTnet.Client;
using MQTTnet.Client.Options;
using TMPro;
using System.Collections;
using Unity.VisualScripting;
using Unity.XR.CoreUtils;
using System.Collections.Generic;
using UnityEngine.InputSystem;
using System.Linq;
using NUnit.Framework;
using UnityEditor;

public class BowData
{
    public bool fired { get; set; }
    public int draw { get; set; }
}

public class MQTTS_Test : MonoBehaviour
{
    [Header("Broker Configuration")]
    [SerializeField] private string host = "10.223.202.48";
    [SerializeField] private int port = 8883;

    [Header("Subscriptions")]
    [SerializeField] private string[] topicsToSubscribe = { "sensor/bow", "u96/commands", "sensor/glove"};

    [Header("UI References")]
    //[SerializeField] private TextMeshProUGUI statusText;
    
    [SerializeField] private int maxLogLines = 30; 

    private IMqttClient client;
    private IMqttClientOptions options;

    // Scrolling log, managed by thread safe queue for safety
    private ConcurrentQueue<string> messageLog = new ConcurrentQueue<string>();
    
    // Safety flag to kill the retry loop when you press "Stop" in Unity
    private bool isRunning = true; 
    public bool isFire = false;
    public bool isReload = false;
    public int strength = 0;
    public int prev_strength = 0;
    public int fire_strength = 0;
    public LogicScript logic;
    public XROrigin xROrigin;
    private bool canCommand = true;
    private bool canGesture = true;
    private bool canFire = true;
    private int json_strength = 0;
    private bool json_isFire = false;
    private ConcurrentQueue<BowData> bow = new ConcurrentQueue<BowData>();
    private ConcurrentQueue<string> glove = new ConcurrentQueue<string>();
    private ConcurrentQueue<string> commands = new ConcurrentQueue<string>();

    // Fires once on startup
    private void Start() {
        logic = GameObject.FindGameObjectWithTag("Logic").GetComponent<LogicScript>();

        //Debug.Log("MQTTS Initializing...");
        
        // Initial text on screen
        //statusText.text = "Connecting to broker...\nWaiting for messages...";

        var factory = new MqttFactory();
        client = factory.CreateMqttClient();

        // Add received messages to our log queue, message callback
        client.UseApplicationMessageReceivedHandler(e => {
            string topic = e.ApplicationMessage.Topic;
            string payload = e.ApplicationMessage.Payload == null
                ? ""
                : Encoding.UTF8.GetString(e.ApplicationMessage.Payload);

            // String format: [Timestamp] topic_name: payload_data
            string logEntry = $"[{DateTime.Now:HH:mm:ss}] {topic}: {payload}";

            // Add to log
            messageLog.Enqueue(logEntry);

            // Remove the oldest message if we exceed the max 
            while (messageLog.Count > maxLogLines){
                messageLog.TryDequeue(out _);
            }

            //Debug.Log($"[MQTTS] RX {topic}: {payload}"); 

            switch (topic)
            {
                case ("sensor/bow"):
                BowData obj = JsonConvert.DeserializeObject<BowData>(payload);
                bow.Enqueue(obj);
                break;
                case ("u96/commands"):
                commands.Enqueue(payload);
                break;
                case ("sensor/glove"):
                glove.Enqueue(payload);
                break;
                default:
                break;
            }
            
        });

        // Fires when connected to broker, callback
        client.UseConnectedHandler(e => {
            //Debug.Log($"[{DateTime.Now:HH:mm:ss}] SYSTEM: Connected to Broker!");
            messageLog.Enqueue($"[{DateTime.Now:HH:mm:ss}] SYSTEM: Connected to Broker!");
        });

        // Fires when disconnected from broker
        client.UseDisconnectedHandler(e => {
            //Debug.Log($"[{DateTime.Now:HH:mm:ss}] SYSTEM: Disconnected. Retrying...");
            messageLog.Enqueue($"[{DateTime.Now:HH:mm:ss}] SYSTEM: Disconnected. Retrying...");
        });

        // Load certificate for authentication and TLS encryption
        TextAsset certData = Resources.Load<TextAsset>("ca_cert");
        var expectedCaCert = new System.Security.Cryptography.X509Certificates.X509Certificate(certData.bytes);

        var tlsParams = new MqttClientOptionsBuilderTlsParameters {
            UseTls = true,
            AllowUntrustedCertificates = false,
            IgnoreCertificateChainErrors = false,
            CertificateValidationHandler = context => {
                    var myCaCert = new System.Security.Cryptography.X509Certificates.X509Certificate2(expectedCaCert);
                    
                    // The chain contains the server cert AND the CA that signed it
                    if (context.Chain != null) {
                        foreach (var chainElement in context.Chain.ChainElements) {
                            if (chainElement.Certificate.Thumbprint == myCaCert.Thumbprint){
                                return true; 
                            }
                        }
                    }

                    // Fallback: If it's a completely self-signed cert acting as its own CA
                    var brokerCert = new System.Security.Cryptography.X509Certificates.X509Certificate2(context.Certificate);
                    if (brokerCert.Thumbprint == myCaCert.Thumbprint) {
                        return true;
                    }

                    //Debug.LogError($"Security Alert: Broker cert (Thumbprint: {brokerCert.Thumbprint}) is not trusted by our CA!");
                    return false;
                }
        };

        options = new MqttClientOptionsBuilder()
            .WithClientId("Visualiser") 
            .WithTcpServer(host, port)
            .WithTls(tlsParams)
            .Build();

        // Initiate async connection loop
        _ = ConnectionLoopAsync();
    }

    // Async connection Loop
    private async Task ConnectionLoopAsync() {
        while (isRunning) {
            // If client is not connected, initiate reconnect
            if (client != null && !client.IsConnected) {
                try {
                    await client.ConnectAsync(options, CancellationToken.None);
                    // Resubscribe topics upon connection
                    if (topicsToSubscribe.Length > 0) {
                        var filters = new MQTTnet.MqttTopicFilter[topicsToSubscribe.Length];
                        for (int i = 0; i < topicsToSubscribe.Length; i++) {
                            filters[i] = new MQTTnet.MqttTopicFilterBuilder().WithTopic(topicsToSubscribe[i]).Build();
                        }
                        await client.SubscribeAsync(filters);
                        messageLog.Enqueue($"[{DateTime.Now:HH:mm:ss}] SYSTEM: Subscribed to topics.");
                    }
                }
                catch (Exception){
                }
            }
            await Task.Delay(2000);
        }
    }

    // Update UI, runs every single frame
    private void Update() {
        // if (statusText == null) return;
        // // Print all the messages in the log queue 
        // if (!messageLog.IsEmpty) {
        //     statusText.text = string.Join("\n", messageLog);
        // }
        if (commands.TryDequeue(out string u96command))
        {
            if (canCommand)
            {
                switch (u96command)
                {
                    case ("restart"):
                        if (logic.PauseScreen.activeSelf || logic.gameOverScreen.activeSelf)
                        {
                            logic.RestartGame();
                        }
                        break;
                    case ("begin"):
                        if (logic.StartButtonScreen.activeSelf || logic.TutorialScreen.activeSelf)
                        {
                            logic.BeginGame();
                        }
                        break;
                    case ("resume"):
                        if (logic.PauseScreen.activeSelf)
                        {
                            logic.Resume();
                        } else if (logic.StartButtonScreen.activeSelf || logic.TutorialScreen.activeSelf)
                        {
                            logic.BeginGame();
                        }
                        break;
                    case ("pause"):
                        if (logic.UI.activeSelf && !logic.PauseScreen.activeSelf && !logic.RestartScreen.activeSelf && !logic.TutorialScreen.activeSelf)
                        {
                            logic.Pause();
                        }
                        break;
                    case ("withdraw"):
                        if (logic.PauseScreen.activeSelf || logic.gameOverScreen.activeSelf)
                        {
                            logic.Quit();
                        }
                        break;
                    case ("water"): // blue
                        if (logic.UI.activeSelf && !logic.PauseScreen.activeSelf)
                        {
                            logic.ChangeArrowToBlue();
                        }
                        break;
                    case ("fire"): // red
                        if (logic.UI.activeSelf && !logic.PauseScreen.activeSelf)
                        {
                            logic.ChangeArrowToRed();
                        }
                        break;
                    case ("lightning"): // yellow
                        if (logic.UI.activeSelf && !logic.PauseScreen.activeSelf)
                        {
                            logic.ChangeArrowToYellow();
                        }
                        break;
                    case ("poison"): // purple
                        if (logic.UI.activeSelf && !logic.PauseScreen.activeSelf)
                        {
                            logic.ChangeArrowToPurple();
                        }
                        break;
                    default:
                        break;
                }
                canCommand = false;
                StartCoroutine(SetCanCommandAfterDelay());
            }
        }

        if (glove.TryDequeue(out string gesture))
        {
            if (canGesture)
            {
                switch (gesture)
                {
                    case ("0"):
                        if (logic.UI.activeSelf && !logic.PauseScreen.activeSelf)
                        {
                            xROrigin.GetComponent<ARFireArrow>().ReloadObject(logic.ArrowType);
                            canFire = false;
                        }
                        break;
                    case ("1"):
                        if (logic.UI.activeSelf && !logic.PauseScreen.activeSelf)
                        {
                            logic.Super();
                        }
                        break;
                    default:
                        break;
                }
                canGesture = false;
                StartCoroutine(SetCanGestureAfterDelay());
            }
        }

        if (bow.TryDequeue(out BowData obj))
        {
            json_isFire = obj.fired;
            json_strength = obj.draw;
        }
        strength = json_strength; //> 4 ? 3 : (json_strength < 1 ? 0 : json_strength - 1);
        if (strength > prev_strength)
        {
            canFire = true;
            //Debug.Log(canFire);
        }
        prev_strength = strength;
        isFire = canFire && json_isFire;
    }

    IEnumerator SetCanCommandAfterDelay()
    {
        yield return new WaitForSecondsRealtime(0.25f);
        canCommand = true;
    }

    IEnumerator SetCanGestureAfterDelay()
    {
        yield return new WaitForSecondsRealtime(0.25f);
        canGesture = true;
    }

    // On app close
    private async void OnDestroy() {
        isRunning = false;
        if (client != null && client.IsConnected) {
            await client.DisconnectAsync();
        }
    }
}