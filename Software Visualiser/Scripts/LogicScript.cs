using System;
using System.Collections;
using System.Collections.Generic;
using NUnit.Framework;
using Unity.XR.CoreUtils;
using UnityEngine;
using UnityEngine.Rendering.Universal.Internal;
using UnityEngine.UI;
using UnityEngine.XR.ARFoundation;

public class LogicScript : MonoBehaviour
{
    public int score;
    private float time = 0;
    public Text scoreText;
    public Text finalScoreText;
    public Text strengthText;
    public Text Timer;
    public Text remainingMarkersText;
    public Image healthBar;
    public Image superBar;
    public Image spawnEnemiesButton;
    public float healthAmount = 100f;
    public GameObject gameOverScreen;
    public GameObject TutorialScreen;
    public GameObject StartScreen;
    public GameObject CalibrationScreen;
    public GameObject StartButtonScreen;
    public GameObject PauseScreen;
    public GameObject RestartScreen;
    public GameObject PauseButton;
    public GameObject UI;
    public XROrigin xROrigin;
    public bool clearEnemy = false;
    private int Arrowtype = 0;
    public int ArrowType
    {
        get {return Arrowtype;}
    }
    [SerializeField] List<Text> arrowTextsToChange = new List<Text>();
    private Dictionary<int, Text> arrowTexts;
    private readonly int BlueArrow = 0;
    private readonly int RedArrow = 1;
    private readonly int YellowArrow = 2;
    private readonly int PurpleArrow = 3;
    private int markersDetected;
    private int numOfPrefabs;
    private int remainingMarkers;
    private bool enableRendering = false;
    public bool EnableRendering
    {
        get { return enableRendering; }
    }
    public bool isTouch = false;
    public bool isGodMode;
    public bool isTutorialMode;
    private float cooldown = 5f;
    private float godCooldown = 1f;
    private float lastTimeCasted = 0;
    public MQTTS_Test mqtt;

    [ContextMenu("Increase Score")]
    public void AddScore(int scoreToAdd)
    {
        score += scoreToAdd;
        scoreText.text = score.ToString();
        finalScoreText.text = "Final Score: " + score.ToString();
    }

    void ResetScore()
    {
        score = 0;
        scoreText.text = score.ToString();
        finalScoreText.text = "Final Score: " + score.ToString();
    }

    public void TakeDamage(float damage)
    {
        if (isGodMode || isTutorialMode)
        {
            return;
        }
        healthAmount -= damage;
        healthBar.fillAmount = healthAmount / 100f;
    }

    public void ChangeArrowToBlue()
    {
        ChangeArrow(BlueArrow);
    }

    public void ChangeArrowToRed()
    {
        ChangeArrow(RedArrow);
    }

    public void ChangeArrowToYellow()
    {
        ChangeArrow(YellowArrow);
    }

    public void ChangeArrowToPurple()
    {
        ChangeArrow(PurpleArrow);
    }

    public void Super()
    {
        if (time < lastTimeCasted + (isGodMode ? godCooldown : cooldown))
        {
            return;
        }

        xROrigin.GetComponent<ARFireArrow>().PlaceSuper();
        lastTimeCasted = time;
    }

    private void ChangeArrow(int Arrow)
    {
        if (xROrigin.GetComponent<ARFireArrow>().HasFired == true)
        {
            arrowTexts[Arrowtype].enabled = false;
            arrowTexts[Arrow].enabled = true;
            Arrowtype = Arrow;
        }
    }

    void Start()
    {
        DisableArrowInput();
        arrowTexts = new Dictionary<int, Text>();
        SetupArrowTexts();
        mqtt = GameObject.FindGameObjectWithTag("Input").GetComponent<MQTTS_Test>();
    }

    private void DisableArrowInput()
    {
        xROrigin.GetComponent<ARFireArrow>().enabled = false;
    }

    private void EnableArrowInput()
    {
        xROrigin.GetComponent<ARFireArrow>().enabled = true;
    }

    private void SetupArrowTexts()
    {
        int i = 0;
        foreach (var text in arrowTextsToChange)
        {   
            if (i == 0)
            {
                text.enabled = true;
            } else
            {
                text.enabled = false;
            }
            arrowTexts.Add(i, text);
            i++;
        }
    }


    void Update()
    {
        int seconds_elapsed = Mathf.FloorToInt(time);
        int seconds = seconds_elapsed % 60;
        int minutes = seconds_elapsed / 60;

        Timer.text = minutes.ToString("00") + ":" + seconds.ToString("00");

        if (healthAmount <= 0)
        {
            DisableArrowInput();
            xROrigin.GetComponent<ARFireArrow>().ClearFakeArrow(Arrowtype);
            Time.timeScale = 0f;
            UI.SetActive(false);
            gameOverScreen.SetActive(true);
        } else
        {
            time += Time.deltaTime;
            gameOverScreen.SetActive(false);
        }

        superBar.fillAmount = (time - lastTimeCasted) / (isGodMode ? godCooldown : cooldown);
        strengthText.text = "Strength: " + mqtt.strength;

        markersDetected = xROrigin.GetComponent<ImageTracker>().ImagesDetected;
        numOfPrefabs = xROrigin.GetComponent<ImageTracker>().NumOfPrefabs;
        remainingMarkers = numOfPrefabs - markersDetected;
    }

    void ResetHealth()
    {
        healthAmount = 100f;
        healthBar.fillAmount = 1;
    }

    public void RestartGameState()
    {
        xROrigin.GetComponent<ARFireArrow>().ClearFakeArrow(Arrowtype);
        ChangeArrowToBlue();
        enableRendering = false;
        PauseScreen.SetActive(false);
        RestartScreen.SetActive(false);
        StartCoroutine(RestartGameAfterDelay());

        Time.timeScale = 1f;
    }

    IEnumerator CalibrationSequence()
    {
        enableRendering = true;
        PauseScreen.SetActive(false);
        StartScreen.SetActive(false);
        CalibrationScreen.SetActive(true);
        while (remainingMarkers != 0)
        {
            //Debug.Log(remainingMarkers);
            remainingMarkersText.text = "Remaining Markers: " + remainingMarkers.ToString();
            yield return null;
        }
        remainingMarkersText.text = "Remaining Markers: 0";
        StartButtonScreen.SetActive(true);
    }

    public void RestartGame()
    {
        isGodMode = false;
        isTutorialMode = false;
        RestartGameState();
    }

    public void RestartMenu()
    {
        PauseScreen.SetActive(false);
        RestartScreen.SetActive(true);
    }

    IEnumerator RestartGameAfterDelay()
    {
        clearEnemy = true;
        xROrigin.GetComponent<ImageTracker>().ResetCoroutines();
        yield return new WaitForSecondsRealtime(0.25f);
        CalibrationScreen.SetActive(false);
        StartButtonScreen.SetActive(false);
        Text godText = healthBar.transform.Find("GODText").GetComponent<Text>();
        Text tutorialText = healthBar.transform.Find("TutorialText").GetComponent<Text>();
        if (isTutorialMode)
        {
            godText.gameObject.SetActive(false);
            tutorialText.gameObject.SetActive(true);
        } else
        {
            godText.gameObject.SetActive(isGodMode);
            tutorialText.gameObject.SetActive(false);
        }
        UI.SetActive(true);

        time = 0;
        lastTimeCasted = 0;
        ResetScore();
        ResetHealth();
        EnableArrowInput();
        if (!isTutorialMode)
        {
            TutorialScreen.SetActive(false);
            PauseButton.SetActive(true);
            clearEnemy = false;

        } else
        {
            TutorialScreen.SetActive(true);
            PauseButton.SetActive(false);
            spawnEnemiesButton.fillAmount = 1;
        }
        Time.timeScale = 1f;
    }

    public void Quit()
    {
        healthAmount = 100f;

        gameOverScreen.SetActive(false);
        PauseScreen.SetActive(false);
        UI.SetActive(false);
        StartScreen.SetActive(true);

        xROrigin.GetComponent<ARFireArrow>().ClearFakeArrow(Arrowtype);
        ChangeArrowToBlue();
        DisableArrowInput();

        clearEnemy = true;
        xROrigin.GetComponent<ImageTracker>().clearPositions();

        Time.timeScale = 1f;
    }

    public void StartGame()
    {
        StartCoroutine(CalibrationSequence());
        
    }

    public void BeginGame()
    {
        isGodMode = false;
        isTutorialMode = false;
        RestartGameState();
        
    }

    public void GodMode()
    {
        isGodMode = true;
        isTutorialMode = false;
        RestartGameState();
    }

    public void TutorialMode()
    {
        isGodMode = false;
        isTutorialMode = true;
        RestartGameState();
    }

    public void SpawnEnemies()
    {
        clearEnemy = !clearEnemy;
        if (clearEnemy)
        {
            xROrigin.GetComponent<ImageTracker>().ResetCoroutines();
            spawnEnemiesButton.fillAmount = 1;
        } else
        {
            spawnEnemiesButton.fillAmount = 0;
        }
    }

    public void StartGamewithTouch()
    {
        StartCoroutine(CalibrationSequence());
        isTouch = true;
    }

    public void Resume()
    {
        PauseScreen.SetActive(false);
        RestartScreen.SetActive(false);
        Time.timeScale = 1f;
        EnableArrowInput();
    }

    public void Pause()
    {
        if (PauseScreen.activeSelf || RestartScreen.activeSelf)
        {
            Resume();
            RestartScreen.SetActive(false);
            return;
        }
        PauseScreen.SetActive(true);
        Time.timeScale = 0f;
        DisableArrowInput();
        
    }
}
