// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SubmarineGameInstance.generated.h"

struct FSubmarineSession;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLoginComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCreateSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFindSessionsComplete, bool, bWasSuccessful);

/**
 * 
 */
UCLASS()
class ANTIQUATEDFUTURE_API USubmarineGameInstance : public UGameInstance
{
	GENERATED_BODY()

	const FString LogInType = "accountportal";
	const FString SearchKeyword = "SubmarineTest";
	const FName SettingKeyLobbyName = "SubmarineLobbyName";
	const FName TestSessionName = FName("Submarine Session");
	
protected:
	class IOnlineSubsystem* OnlineSubsystem;
	TSharedPtr<class FOnlineSessionSearch> SessionSearch;

	static void LogNoSubsystem();
	static void LogNoSessionInterface();
	static void LogNoIdentityInterface();
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnLoginComplete(
		int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& ErrorMessage);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

public:
	USubmarineGameInstance();
	
	virtual void Init() override;

	UPROPERTY(BlueprintReadOnly)
	bool bIsSearching;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsLan;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAdvertise;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int NumPlayers;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUsePresence;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseLobbies;
	
	UFUNCTION(BlueprintCallable)
	int GetNumSessionsFound();
	UFUNCTION(BlueprintCallable)
	bool IsLoggedIn();
	UFUNCTION(BlueprintCallable)
	bool NeedsLogIn() { return !IsLoggedIn(); }
	UFUNCTION(BlueprintCallable)
	void LogIn(const int PlayerNumber = 0);
	UFUNCTION(BlueprintCallable)
	void CreateSession(const FString& SessionName);
	UFUNCTION(BlueprintCallable)
	void FindSessions();
	UFUNCTION(BlueprintCallable)
	void JoinSession(int SessionNumber);

	UFUNCTION(BlueprintCallable)
	TArray<FSubmarineSession> GetSearchResults();

	UPROPERTY(BlueprintAssignable)
	FLoginComplete LogInCompleted;
	UPROPERTY(BlueprintAssignable)
	FFindSessionsComplete FindSessionsCompleted;
	UPROPERTY(BlueprintAssignable)
	FCreateSessionComplete CreateSessionsCompleted;
	
};
