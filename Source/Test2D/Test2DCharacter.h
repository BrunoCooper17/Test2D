// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PaperCharacter.h"
#include "Test2DCharacter.generated.h"

class UTextRenderComponent;

UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class EAnimationsEnum : uint8 {
	RUN 			UMETA(DisplayName="Run"),
	IDLE 			UMETA(DisplayName="Idle"),
	JUMP_START		UMETA(DisplayName="JumpStart"),
	JUMP_END		UMETA(DisplayName="JumpEnd"),
	DIE_START		UMETA(DisplayName="DieStart"),
	DIE_END			UMETA(DisplayName="DieEnd"),
	FIRE_SHOT		UMETA(DisplayName="FireShot"),
	FALLING			UMETA(DisplayName="Falling"),
	SIZE
};

USTRUCT(BlueprintType)
struct FAnimData {
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere)
	class UPaperFlipbook* Animation;
	UPROPERTY(EditAnywhere)
	bool bLoopAnim;
};

USTRUCT(BlueprintType)
struct FAnimList {
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere)
	FAnimData AnimData[(int32)EAnimationsEnum::SIZE];
};

/**
 * This class is the default character for Test2D, and it is responsible for all
 * physical interaction between the player and the world.
 *
 * The capsule component (inherited from ACharacter) handles collision with the world
 * The CharacterMovementComponent (inherited from ACharacter) handles movement of the collision capsule
 * The Sprite component (inherited from APaperCharacter) handles the visuals
 */
UCLASS(config=Game)
class ATest2DCharacter : public APaperCharacter
{
	GENERATED_BODY()

	/** Side view camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera, meta=(AllowPrivateAccess="true"))
	class UCameraComponent* SideViewCameraComponent;

	/** Camera boom positioning the camera beside the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UTextRenderComponent* TextComponent;
	virtual void Tick(float DeltaSeconds) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "My Values", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* FloorDetectionSensor;
	
protected:
	// The animation to play while running around
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="My Values")
	FAnimList AnimationsList;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="My Values")
	EAnimationsEnum CurrentAnimation = EAnimationsEnum::IDLE;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="My Values")
	EAnimationsEnum NextStateAnimation = EAnimationsEnum::IDLE;
	
	virtual void BeginPlay() override;

	/** Called to choose the correct animation to play based on the character's movement state */
	void UpdateAnimation();

	/** Called for side to side input */
	void MoveRight(float Value);

	void UpdateCharacter();

	/** Handle touch inputs. */
	void TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location);

	/** Handle touch stop event. */
	void TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// End of APawn interface
	
	UFUNCTION(BlueprintCallable, Category="MyFunctions")
	virtual void Landed(const FHitResult& Hit) override;
	UFUNCTION(BlueprintCallable, Category="MyFunctions")
	virtual void Falling() override;
	
	EAnimationsEnum GetNextState(EAnimationsEnum LastState);
	
	bool bFalling;

public:
	ATest2DCharacter();
	
	UFUNCTION(BlueprintCallable, Category="MyFunctions")
	void DoubleJump();

	/** Returns SideViewCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetSideViewCameraComponent() const { return SideViewCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
};
