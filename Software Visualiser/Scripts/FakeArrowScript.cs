using UnityEngine;

public class FakeArrowScript : MonoBehaviour
{
    public ARFireArrow aRFireArrow;
    private float maxZ = 0.3f;
    private Vector3 defaultPos;
    
    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start()
    {
        aRFireArrow = FindFirstObjectByType<ARFireArrow>();
        defaultPos = transform.localPosition;
    }

    // Update is called once per frame
    void Update()
    {
        float strength = aRFireArrow.Strength * maxZ;

        if (strength > 0)
        {   
            strength = strength > maxZ ? maxZ : strength;
            transform.localPosition = defaultPos + (strength * Vector3.back);
        } else
        {
            transform.localPosition = defaultPos;
        }
    }
}
