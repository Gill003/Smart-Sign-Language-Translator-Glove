import * as THREE from 'three'
import { OrbitControls } from 'three/addons/controls/OrbitControls.js'
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js'
import { OutlineEffect } from 'three/examples/jsm/effects/OutlineEffect.js'
const PARAMS = {
  bg: 0x4b46b2,
  hand: 0xE7A183,
  shirt: 0x303030,
  vest: 0xE7D55C,
  wrist: 0.1,
  thumb: 0.25,
  index: 0.25,
  middle: 1.1,
  ring: 1.1,
  pinky: 0.25,
  thumbz: -0.15, 
  indexz: -0.3,
  middlez: -0.08,
  ringz: -0.22,
  pinkyz: -0.52
}
if (!!window.EventSource) {
  var source = new EventSource('/events');
}
source.addEventListener('open', function(e) {
  console.log("Events Connected");
}, false);

source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
}, false);
source.addEventListener('prediction', function(e) {
  console.log("prediction", e.data);
  var obj = JSON.parse(e.data);
  let prediction = obj.pred;

  // Update the prediction text in the HTML
  document.getElementById('prediction').innerHTML = "Prediction: " + prediction;
  
  if(document.getElementById('muteButton').textContent == 'Mute')
    speakText(prediction);
  
}, false);

// Canvas
const canvas = document.querySelector('canvas.webgl')

// Scene
const scene = new THREE.Scene()
const bgColor = new THREE.Color(PARAMS.bg)
scene.background = bgColor


const gltfLoader = new GLTFLoader()

gltfLoader.load(
  'hand.glb',
  (gltf) =>
  {
    const hand = gltf.scene;

    // Flip the hand to make it right-handed
    hand.scale.set(-1, 1, 1)
    hand.traverse((child) => {
      if (child.isMesh) {
        child.material.side = THREE.DoubleSide; // Handle flipped normals
      }
    });
    scene.add(hand)
    
    setMaterials()
    setBones()
  }
)

// Materials
const handMaterial = new THREE.MeshToonMaterial()
const shirtMaterial = new THREE.MeshToonMaterial()
const vestMaterial = new THREE.MeshToonMaterial()

const setMaterials = () => {
  const textureLoader = new THREE.TextureLoader()
  const gradientTexture = textureLoader.load('3.jpg')
  gradientTexture.minFilter = THREE.NearestFilter
  gradientTexture.magFilter = THREE.NearestFilter
  gradientTexture.generateMipmaps = false

  handMaterial.color = new THREE.Color(PARAMS.hand)
  handMaterial.gradientMap = gradientTexture
  handMaterial.roughness = 0.7
  handMaterial.emissive = new THREE.Color(PARAMS.hand)
  handMaterial.emissiveIntensity = 0.2
  scene.getObjectByName('Hand').material = handMaterial

  shirtMaterial.color = new THREE.Color(PARAMS.shirt)
  shirtMaterial.gradientMap = gradientTexture
  scene.getObjectByName('Shirt').material = shirtMaterial

  vestMaterial.color = new THREE.Color(PARAMS.vest)
  vestMaterial.gradientMap = gradientTexture
  scene.getObjectByName('Vest').material = vestMaterial

}

const setBones = () => {
  const hand = scene.getObjectByName('Hand'); // Get the entire Hand object

  hand.parent.rotation.x = Math.PI / 2; // Rotate around x-axis Math.PI / 2=90deg
  hand.parent.rotation.y = 0; // Rotate around y-axis
  hand.parent.rotation.z = Math.PI ; // Rotate around z-axis
  const wrist = scene.getObjectByName('Hand').skeleton.bones[0]
  const wrist1 = scene.getObjectByName('Hand').skeleton.bones[1]
  const wrist2 = scene.getObjectByName('Hand').skeleton.bones[2]
  const wrist3 = scene.getObjectByName('Hand').skeleton.bones[6]
  const wrist4 = scene.getObjectByName('Hand').skeleton.bones[10]
  const wrist5 = scene.getObjectByName('Hand').skeleton.bones[14]
  const wrist6 = scene.getObjectByName('Hand').skeleton.bones[18]
  wrist1.scale.set(1, 1.1,1)
  wrist1.rotation.x = PARAMS.wrist
  wrist2.rotation.x = PARAMS.wrist
  wrist3.rotation.x = PARAMS.wrist
  wrist4.rotation.x = PARAMS.wrist
  wrist5.rotation.x = PARAMS.wrist
  wrist6.rotation.x = PARAMS.wrist

  const thumb1 = scene.getObjectByName('Hand').skeleton.bones[3]
  const thumb2 = scene.getObjectByName('Hand').skeleton.bones[4]
  const thumb3 = scene.getObjectByName('Hand').skeleton.bones[5]
  thumb1.scale.set(0.9, 1.3, 0.9)
  thumb1.rotation.x = PARAMS.thumb
  thumb2.rotation.x = PARAMS.thumb
  thumb3.rotation.x = PARAMS.thumb
  thumb1.rotation.z = PARAMS.thumbz
  thumb2.rotation.z = PARAMS.thumbz
  thumb3.rotation.z = PARAMS.thumbz

  const index1 = scene.getObjectByName('Hand').skeleton.bones[7]
  const index2 = scene.getObjectByName('Hand').skeleton.bones[8]
  const index3 = scene.getObjectByName('Hand').skeleton.bones[9]
  index1.scale.set(0.9, 1.3, 0.9)
  index1.rotation.x = PARAMS.index
  index2.rotation.x = PARAMS.index
  index3.rotation.x = PARAMS.index

  const middle1 = scene.getObjectByName('Hand').skeleton.bones[11]
  const middle2 = scene.getObjectByName('Hand').skeleton.bones[12]
  const middle3 = scene.getObjectByName('Hand').skeleton.bones[13]
  middle1.scale.set(0.9, 1.3, 0.9)
  middle1.rotation.x = PARAMS.middle
  middle2.rotation.x = PARAMS.middle
  middle3.rotation.x = PARAMS.middle

  const ring1 = scene.getObjectByName('Hand').skeleton.bones[15]
  const ring2 = scene.getObjectByName('Hand').skeleton.bones[16]
  const ring3 = scene.getObjectByName('Hand').skeleton.bones[17]
  ring1.scale.set(0.9, 1.3, 0.9)
  ring1.rotation.x = PARAMS.ring
  ring2.rotation.x = PARAMS.ring
  ring3.rotation.x = PARAMS.ring

  const pinky1 = scene.getObjectByName('Hand').skeleton.bones[19]
  const pinky2 = scene.getObjectByName('Hand').skeleton.bones[20]
  const pinky3 = scene.getObjectByName('Hand').skeleton.bones[21]
  pinky1.scale.set(0.9, 1.3, 0.9)
  pinky1.rotation.x = PARAMS.pinky
  pinky2.rotation.x = PARAMS.pinky
  pinky3.rotation.x = PARAMS.pinky

  const wristRotation = [wrist.rotation, wrist1.rotation, wrist2.rotation, wrist3.rotation, wrist4.rotation, wrist5.rotation, wrist6.rotation]
  const thumbRotation = [thumb1.rotation, thumb2.rotation, thumb3.rotation]
  const indexRotation = [index1.rotation, index2.rotation, index3.rotation]
  const middleRotation = [middle1.rotation, middle2.rotation, middle3.rotation]
  const ringRotation = [ring1.rotation, ring2.rotation, ring3.rotation]
  const pinkyRotation = [pinky1.rotation, pinky2.rotation, pinky3.rotation]

  

 
  
    source.addEventListener('flex_sensor', function(e) {
      console.log("flex_sensor", e.data);
      var obj = JSON.parse(e.data);
      let thumb = obj.thumb;
      let Index = obj.Index;
      let Middle = obj.Middle;
      let Ring = obj.Ring;
      let Pinky = obj.Pinky;
     
      let Xrotation=obj.gyroX;
      let Yrotation=obj.gyroY;
      let Zrotation=obj.gyroZ;
    
   
      
      // Change cube rotation after receiving the readinds
      
      const HandMotion = gsap.timeline()
     
      hand.parent.rotation.x 
    
      HandMotion
      .to(hand.parent.rotation, {
        duration: 0.5,
        x: -Xrotation , 
        y: -Yrotation,
        z: Zrotation 
      }, 'same')
        .to(wristRotation, { duration: 0.5, x: 0 }, 'same')
        .to(wristRotation, { duration: 0.5, x: 0 }, 'same')
        .to(thumbRotation, { duration: 0.5, x: thumb }, 'same')
        .to(indexRotation, { duration: 0.5, x: Index}, 'same')
        .to(middleRotation, { duration: 0.5, x: Middle }, 'same')
        .to(ringRotation, { duration: 0.5, x: Ring }, 'same')
        .to(pinkyRotation, { duration: 0.5, x: Pinky }, 'same')
        .to(thumbRotation, { duration: 0.5, z: -0.15 }, 'same')
        .to(indexRotation[0], { duration: 0.5, z: -0.30 }, 'same')
        .to(middleRotation[0], { duration: 0.5, z: -0.08 }, 'same')
        .to(ringRotation[0], { duration: 0.5, z: 0.22 }, 'same')
        .to(pinkyRotation[0], { duration: 0.5, z: 0.52 }, 'same')
        
        .play()
      });
      
}

/**
 * Lights
 */
const ambientLight = new THREE.AmbientLight(0xffffff, 0.3)
scene.add(ambientLight)

const directionalLight = new THREE.DirectionalLight(0xffffff, 2)
directionalLight.position.set(-5, 5, 5)
directionalLight.scale.set(0.5, 0.5, 0.5)
scene.add(directionalLight)

/**
 * Sizes
 */
const sizes = {
  width: window.innerWidth,
  height: window.innerHeight
}

window.addEventListener('resize', () =>
{
  // Update sizes
  sizes.width = window.innerWidth
  sizes.height = window.innerHeight

  // Update camera
  camera.aspect = sizes.width / sizes.height
  camera.updateProjectionMatrix()

  // Update renderer
  outlineEffect.setSize(sizes.width, sizes.height)
  outlineEffect.setPixelRatio(Math.min(window.devicePixelRatio, 2))
})

function resetPosition(element){
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/"+element.id, true);
  console.log(element.id);
  xhr.send();
  document.getElementById('prediction').innerHTML = "Prediction: ";
}
window.resetPosition = resetPosition;

function getPrediction(element) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/"+element.id, true);
  console.log(element.id);
  xhr.send();
  // Update the prediction text in the HTML
  document.getElementById('prediction').innerHTML = "Prediction: " + ".....";
  //if.document.getElementById()
  
}
window.getPrediction = getPrediction;

function speakText(text) {
  
  var utterance = new SpeechSynthesisUtterance(text);

  // Set the language directly to English (US)
  utterance.lang = 'en-US';

  // Optional: Set pitch, rate, and volume for clearer speech
  utterance.pitch = 1.2;  // Slightly higher pitch
  utterance.rate = 1;     // Normal speed
  utterance.volume = 1;   // Maximum volume

  // Speak the text
  speechSynthesis.speak(utterance);
}
window.speakText = speakText;

function muteChange(element){
  if (element.textContent == "Unmute") {
      element.textContent = "Mute";  // Change button text to Mute
      element.classList.remove('muted');  // Remove muted class
  } else {
      muteButton.textContent = "Unmute";  // Change button text to Unmute
      muteButton.classList.add('muted');  // Add muted class
  }
}
window.muteChange = muteChange;


/**
 * Camera
 */
const camera = new THREE.PerspectiveCamera(75, sizes.width / sizes.height, 0.1, 100)
camera.position.set(0, 0, 5)
scene.add(camera)

// Controls
const controls = new OrbitControls(camera, canvas)
controls.target.set(0, 0, 0)
controls.enableDamping = true
controls.maxPolarAngle = Math.PI / 2
controls.minDistance = 3
controls.maxDistance = 10

/**
 * Renderer
 */
const renderer = new THREE.WebGLRenderer({
  canvas: canvas,
  alpha: true,
})
renderer.shadowMap.enabled = true
renderer.shadowMap.type = THREE.PCFSoftShadowMap
renderer.setSize(sizes.width, sizes.height)
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2))

const outlineEffect = new OutlineEffect(renderer, {
  defaultThickness: 0.0035,
  defaultColor: [ 0, 0, 0 ],
  defaultAlpha: 0.8,
  defaultKeepAlive: true
})


/**
 * Animate
 */
const clock = new THREE.Clock()
let previousTime = 0

const tick = () =>
{
    const elapsedTime = clock.getElapsedTime()
    const deltaTime = elapsedTime - previousTime
    previousTime = elapsedTime

    // Update controls
    controls.update()

    // Render
    outlineEffect.render(scene, camera)

    // Call tick again on the next frame
    window.requestAnimationFrame(tick)
}


tick()