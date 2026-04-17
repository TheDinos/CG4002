using UnityEngine;

public class ARPlaneScript : MonoBehaviour
{
    public MeshRenderer meshRenderer;
    public LineRenderer lineRenderer;
    public LogicScript logic;
    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start()
    {
        logic = GameObject.FindGameObjectWithTag("Logic").GetComponent<LogicScript>();
    }

    void Update()
    {
        if (logic.EnableRendering == true)
        {
            enableRendering();
        } else
        {
            disableRendering();
        }
    }
    void disableRendering()
    {
        meshRenderer.enabled = false;
        lineRenderer.enabled = false;
    }

    void enableRendering()
    {
        meshRenderer.enabled = true;
        lineRenderer.enabled = true;
    }
}
