// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Test2DCharacter.h"
#include "PaperFlipbookComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Camera/CameraComponent.h"

DEFINE_LOG_CATEGORY_STATIC(SideScrollerCharacter, Log, All);

//////////////////////////////////////////////////////////////////////////
// ATest2DCharacter

ATest2DCharacter::ATest2DCharacter()
{
	// Use only Yaw from the controller and ignore the rest of the rotation.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Set the size of our collision capsule.
	GetCapsuleComponent()->SetCapsuleHalfHeight(96.0f);
	GetCapsuleComponent()->SetCapsuleRadius(40.0f);

	// Create a camera boom attached to the root (capsule)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 500.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 75.0f);
	CameraBoom->bAbsoluteRotation = true;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->RelativeRotation = FRotator(0.0f, -90.0f, 0.0f);

	// Create an orthographic camera (no perspective) and attach it to the boom
	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;
	SideViewCameraComponent->OrthoWidth = 2048.0f;
	SideViewCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// Prevent all automatic rotation behavior on the camera, character, and camera component
	CameraBoom->bAbsoluteRotation = true;
	SideViewCameraComponent->bUsePawnControlRotation = false;
	SideViewCameraComponent->bAutoActivate = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// Configure character movement
	GetCharacterMovement()->GravityScale = 2.0f;
	GetCharacterMovement()->AirControl = 0.80f;
	GetCharacterMovement()->JumpZVelocity = 1000.f;
	GetCharacterMovement()->GroundFriction = 3.0f;
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->MaxFlySpeed = 600.0f;

	// Lock character motion onto the XZ plane, so the character can't move in or out of the screen
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0.0f, -1.0f, 0.0f));

	// Behave like a traditional 2D platformer character, with a flat bottom instead of a curved capsule bottom
	// Note: This can cause a little floating when going up inclines; you can choose the tradeoff between better
	// behavior on the edge of a ledge versus inclines by setting this to true or false
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;

    // 	TextComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("IncarGear"));
    // 	TextComponent->SetRelativeScale3D(FVector(3.0f, 3.0f, 3.0f));
    // 	TextComponent->SetRelativeLocation(FVector(35.0f, 5.0f, 20.0f));
    // 	TextComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    // 	TextComponent->SetupAttachment(RootComponent);
	
	FloorDetectionSensor = CreateDefaultSubobject<UBoxComponent>(TEXT("FloorSensor"));
	FloorDetectionSensor->SetupAttachment(RootComponent);

	// Enable replication on the Sprite component so animations show up when networked
	GetSprite()->SetIsReplicated(true);
	bReplicates = true;
}

void ATest2DCharacter::BeginPlay() {
	Super::BeginPlay();
	
	//UE_LOG(LogClass, Log, TEXT("BEGIN PLAY"));
	//JumpsLeft = Jumps;
}

//////////////////////////////////////////////////////////////////////////
// Animation

void ATest2DCharacter::UpdateAnimation()
{
	UPaperFlipbookComponent* tmpSprite = GetSprite();
	const FVector PlayerVelocity = GetVelocity();
	const float PlayerSpeedSqr = PlayerVelocity.SizeSquared();
	
	if(tmpSprite->IsPlaying() == false) {
		CurrentAnimation = NextStateAnimation;
		NextStateAnimation = GetNextState(NextStateAnimation);
	}
	else if(tmpSprite->IsLooping()) {
		CurrentAnimation = (PlayerSpeedSqr > 0.0f) ? EAnimationsEnum::RUN : EAnimationsEnum::IDLE;
	}

	//UE_LOG(LogClass, Log, TEXT("PLAYING? %d"), (int32)CurrentAnimation);

	// Are we moving or standing still?
	UPaperFlipbook* DesiredAnimation = AnimationsList.AnimData[(int32)CurrentAnimation].Animation;
	if( tmpSprite->GetFlipbook() != DesiredAnimation )
	{
		//UE_LOG(LogClass, Log, TEXT("PLAYING? %d"), (int32)CurrentAnimation);
		tmpSprite->SetFlipbook(DesiredAnimation);
		tmpSprite->SetLooping(AnimationsList.AnimData[(int32)CurrentAnimation].bLoopAnim);
		
		//if(AnimationsList.AnimData[(int32)CurrentAnimation].bLoopAnim == false) {
			tmpSprite->PlayFromStart();
		//}
	}
}

void ATest2DCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	UpdateCharacter();
}


//////////////////////////////////////////////////////////////////////////
// Input

void ATest2DCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Note: the 'Jump' action and the 'MoveRight' axis are bound to actual keys/buttons/sticks in DefaultInput.ini (editable from Project Settings..Input)
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ATest2DCharacter::DoubleJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAxis("MoveRight", this, &ATest2DCharacter::MoveRight);

	PlayerInputComponent->BindTouch(IE_Pressed, this, &ATest2DCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ATest2DCharacter::TouchStopped);
}

void ATest2DCharacter::MoveRight(float Value)
{
	/*UpdateChar();*/

	// Apply the input to the character motion
	AddMovementInput(FVector(1.0f, 0.0f, 0.0f), Value);
}

void ATest2DCharacter::TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// Jump on any touch
	DoubleJump();
}

void ATest2DCharacter::TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// Cease jumping once touch stopped
	StopJumping();
}

void ATest2DCharacter::UpdateCharacter()
{
	// Update animation to match the motion
	UpdateAnimation();

	// Now setup the rotation of the controller based on the direction we are travelling
	const FVector PlayerVelocity = GetVelocity();	
	float TravelDirection = PlayerVelocity.X;
	// Set the rotation so that the character faces his direction of travel.
	if (Controller != nullptr)
	{
		if (TravelDirection < 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
		}
		else if (TravelDirection > 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
		}
	}
}

void ATest2DCharacter::Landed(const FHitResult& Hit) {
	UE_LOG(LogClass, Log, TEXT("LANDED!"));
	//JumpsLeft = Jumps;
	UPaperFlipbookComponent* tmpSprite = GetSprite();
	tmpSprite->SetFlipbook(AnimationsList.AnimData[(int32)EAnimationsEnum::JUMP_END].Animation);
	tmpSprite->SetLooping(AnimationsList.AnimData[(int32)EAnimationsEnum::JUMP_END].bLoopAnim);
	tmpSprite->PlayFromStart();
	
	CurrentAnimation = EAnimationsEnum::JUMP_END;
	NextStateAnimation = EAnimationsEnum::IDLE;
}

void ATest2DCharacter::Falling() {
	UE_LOG(LogClass, Log, TEXT("FALLING!"));
	if(CurrentAnimation != EAnimationsEnum::JUMP_START && CurrentAnimation != EAnimationsEnum::JUMP_END) {
		CurrentAnimation = NextStateAnimation = EAnimationsEnum::FALLING;
		
		UPaperFlipbookComponent* tmpSprite = GetSprite();
		tmpSprite->SetFlipbook(AnimationsList.AnimData[(int32)EAnimationsEnum::FALLING].Animation);
		tmpSprite->SetLooping(AnimationsList.AnimData[(int32)EAnimationsEnum::FALLING].bLoopAnim);
	}
}

EAnimationsEnum ATest2DCharacter::GetNextState(EAnimationsEnum LastState) {
	switch(LastState) {
		case EAnimationsEnum::JUMP_START:
			return EAnimationsEnum::JUMP_START;
		case EAnimationsEnum::JUMP_END:
			return EAnimationsEnum::IDLE;
			
		case EAnimationsEnum::FALLING:
			return EAnimationsEnum::FALLING;
		case EAnimationsEnum::FIRE_SHOT:
			return EAnimationsEnum::DIE_END;
			
		case EAnimationsEnum::DIE_START:
			return EAnimationsEnum::DIE_END;
		case EAnimationsEnum::DIE_END:
			return EAnimationsEnum::DIE_END;
	}
	
	return EAnimationsEnum::IDLE;
}

void ATest2DCharacter::DoubleJump() {
	//UE_LOG(LogClass, Log, TEXT("Jumps DONE! -> %d"), JumpCurrentCount);
	Jump();
	
	UPaperFlipbookComponent* tmpSprite = GetSprite();
	tmpSprite->SetFlipbook(AnimationsList.AnimData[(int32)EAnimationsEnum::JUMP_START].Animation);
	tmpSprite->SetLooping(AnimationsList.AnimData[(int32)EAnimationsEnum::JUMP_START].bLoopAnim);
	tmpSprite->PlayFromStart();
	
	CurrentAnimation = NextStateAnimation = EAnimationsEnum::JUMP_START;
}